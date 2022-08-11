#include "avr/sleep.h"

// DS1305 registers
#define CLOCK_REG_TIME     0x00  // 7 bytes
#define CLOCK_REG_ALARM0   0x07  // 4 bytes
#define CLOCK_REG_ALARM1   0x0b  // 4 bytes
#define CLOCK_REG_CONTROL  0x0f
#define CLOCK_REG_STATUS   0x10
#define CLOCK_REG_CHARGE   0x11
#define CLOCK_REG_RESERVED 0x12
#define CLOCK_REG_USER_RAM 0x11  // 96 bytes

#define CLOCK_CONTROL_nEOSC (1u<<7)  // Oscillator enable
#define CLOCK_CONTROL_WP    (1u<<6)  // Write protect
#define CLOCK_CONTROL_INTCN (1u<<2)  // Set => Alarm1 raisess IRQ 1
#define CLOCK_CONTROL_AIE1  (1u<<1)  // Enable IRQ 1
#define CLOCK_CONTROL_AIE0  (1u<<0)  // Enable IRQ 0

#define CLOCK_STATUS_IRQF0  (1u<<0)  // IRQ 0 active.
#define CLOCK_STATUS_IRQF1  (1u<<1)  // IRQ 1 active.

//---------------------------------------------------------------------------
// Static workspace
//---------------------------------------------------------------------------
static          ThreeWire ds1305;
static volatile bool      wakeup = false;
static volatile int       irqs   = 0;

//---------------------------------------------------------------------------
// Debugging utilities.
//---------------------------------------------------------------------------
#if CFG_ENABLE_LOGGING

#define IF_LOGGING(x) (x)

void print_init() {
  Serial.begin(9600);
  while (!Serial) {};
  Serial.println("");
  Serial.println(" ---------- Starting ----------");
}

void print_fmt(uint8_t const* blk, char const* fmt) {
  uint8_t value = 0;
  char const* cmd_ptr = fmt;
  while (*cmd_ptr) {
    uint8_t const cmd = *cmd_ptr++;
    if      (cmd >= '0' && cmd <= '9') value = blk[cmd - '0'];
    else if (cmd == 'L'            ) Serial.print(value & 15, HEX);
    else if (cmd == 'H'            ) Serial.print((value >> 4) & 3, HEX);
    else if (cmd == 'U'            ) Serial.print(value >> 4, HEX);
    else if (cmd == 'M'            ) Serial.print((value & 0x80) ? 'M' : 'm');
    else if (cmd == '\\'           ) Serial.println("");
    else                             Serial.print((char)cmd);

  }
}

void print_time() {
  uint8_t blk[7];
  ds1305.read_block(0x0, blk, sizeof(blk));
  print_fmt(blk, "Time:     2HL:1UL:0UL 3L, 4UL/5UL/6UL\\");
}

void print_alarm(uint8_t number) {
  uint8_t blk[5];
  blk[4] = number;
  ds1305.read_block(7 + (number << 2), blk, sizeof(blk) - 1);
  print_fmt(blk, "Alarm 4L: 2HL:1UL:0UL, mask: 0M1M2M3M UL\\");
}

void print_status() {
  uint8_t blk[3];
  ds1305.read_block(0x0f, blk, sizeof(blk) - 1);
  blk[2] = digitalRead(CB_CLOCK_IRQ_PIN);
  print_fmt(blk, "Control: 0UL, Status: 1UL, IRQ: 2UL ");
  print_time();
}

#else
#define IF_LOGGING(x) /* Nothing */
#endif

//---------------------------------------------------------------------------
// Control of LED
//---------------------------------------------------------------------------
#if CFG_ENABLE_LED
void led_setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void led_write(int value) {
  digitalWrite(LED_PIN, value);
}
#else
void led_setup() {
}

void led_write(int value) {
}
#endif

//---------------------------------------------------------------------------
// Control of clock relays
//---------------------------------------------------------------------------
void relays_setup() {
  pinMode(CB_RELAY_A_PIN, OUTPUT);
  pinMode(CB_RELAY_B_PIN, OUTPUT);

  digitalWrite(CB_RELAY_A_PIN, LOW);
  digitalWrite(CB_RELAY_B_PIN, LOW);
}

void relays_toggle() {
  IF_LOGGING(Serial.println("Relay A on."));
  if (CFG_ENABLE_RELAYS) digitalWrite(CB_RELAY_A_PIN, HIGH);
  delay(1000 * CB_RELAY_SEC_ON);
  digitalWrite(CB_RELAY_A_PIN, LOW);
  IF_LOGGING(Serial.println("Relay A off."));

  delay(1000 * CB_RELAY_SEC_GAP);

  IF_LOGGING(Serial.println("Relay B on."));
  if (CFG_ENABLE_RELAYS) digitalWrite(CB_RELAY_B_PIN, HIGH);
  delay(1000 * CB_RELAY_SEC_ON);
  digitalWrite(CB_RELAY_B_PIN, LOW);
  IF_LOGGING(Serial.println("Relay B off."));
}

//---------------------------------------------------------------------------
// DS1305
//---------------------------------------------------------------------------
void ds1305_setup() {
  uint8_t initial_state[] = {
    0x01, 0x00, 0x00, 0x01, 0x27, 0x02, 0x79, // 00-06: Current time, 24h mode.
    0x00, 0x00, 0x12, 0x81,                   // 07-0a: Alarm 0 (12:00:00, daily).
    0x00, 0x00, 0x00, 0x81                    // 0b-0e: Alarm 1 (00:00:00, daily).
  };

  // Clear write protect bit, disable IRQs, oscillator off.
  ds1305.write_byte(CLOCK_REG_CONTROL, CLOCK_CONTROL_nEOSC);

  // Set clock/alarms; also clears IRQs.
  ds1305.write_block(0x00, initial_state, sizeof(initial_state));

  // Disable trickle charge
  ds1305.write_byte(CLOCK_REG_CHARGE, 0);
}

void ds1305_start() {
  // Enable IRQs and start oscillator.
  ds1305.write_byte(CLOCK_REG_CONTROL, CLOCK_CONTROL_AIE0 | CLOCK_CONTROL_AIE1);
}

//---------------------------------------------------------------------------
// Power saving
//---------------------------------------------------------------------------
#if CFG_ENABLE_POWERSAVE
void power_setup() {
  // For ATtiny85 disabling ADC reduces powerdown current from 0.31mA to
  // approx 0.3-0.4 uA. Also necessary to allow internal bandgap referehce
  // to be turned off during sleep.
  ADCSRA = 0;

  // Disable analogue comparator and ensure not using internal
  // bandgap reference. Allowing bandgap reference to be turned
  // off during sleep.
  ACSR = 0b10000000;

  PCMSK = 0b00001000; // Unmask pin-change IRQ on 3.
  GIMSK = 0b00100000; // Enable pin change IRQ on 0-5
}

void power_down() {
  noInterrupts();

  uint8_t old_MCUCR = MCUCR;

  // Enable sleep using power-down.
  MCUCR = (MCUCR & 0b11000111) | 0b00110000;

  // Enter sleep with brown-out detection disabled to allow internal
  // bandgap reference to be diabled, saving a few uA. Timing of steps
  // critical.
  MCUCR |= 0b10000100; // 1. Set BODS and BODSE
  MCUCR &= 0b11111011; // 2. Clear BODSE within 4 clock cycles.
  interrupts();        // 3.
  sleep_cpu();         // 4. Must occur within 3 clock cycles of step 2.

  // Restore previous settings, in particular ensure sleep disabled.
  noInterrupts();
  MCUCR = old_MCUCR & 0b11011111;
  interrupts();
}

#else

void power_setup() {
}

void power_down() {
  led_write(HIGH);
  delay(50);
  led_write(LOW);
  delay(950);
}

#endif


//---------------------------------------------------------------------------
// IRQ handling
//---------------------------------------------------------------------------
void irq_setup() {
  // DS1305 has open drain IRQ outputs.
  pinMode(CB_CLOCK_IRQ_PIN, INPUT_PULLUP);
  wakeup = false;
}

void irq_reset() {
  // Read/write to any Alarm registers to clear IRQ.
  (void)ds1305.read_byte(CLOCK_REG_ALARM0);
  (void)ds1305.read_byte(CLOCK_REG_ALARM1);
  wakeup = false;
}

void irq_handler() {
  wakeup = true;
}

//---------------------------------------------------------------------------
// Initialisation
//---------------------------------------------------------------------------
void setup() {
  IF_LOGGING(print_init());

  ds1305 = ThreeWire(CB_CLOCK_CE_PIN,
                     CB_CLOCK_CLK_PIN,
                     CB_CLOCK_DATA_PIN);

  led_setup();
  relays_setup();
  power_setup();

  irq_setup();
  ds1305_setup();

  irq_start();
  ds1305_start();

  IF_LOGGING(print_alarm(0));
  IF_LOGGING(print_alarm(1));
}

//---------------------------------------------------------------------------
// Main loop
//---------------------------------------------------------------------------
void loop() {
  power_down();

  IF_LOGGING(print_status());

  if (wakeup) {
    IF_LOGGING(Serial.println("***** Clock IRQ *****"));
    irq_reset();
    led_write(HIGH);
    relays_toggle();
    delay(2000);
    irq_reset();
  }
}
