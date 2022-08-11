//---------------------------------------------------------------------------
// Sketch configuration
//---------------------------------------------------------------------------
// Socket for ATtiny85 on the board is wired as follows:
//
//                   +--v--+
//       nIRQ --PB5--|1   8|--VCC
//    Relay A --PB3--|2   7|--PB2-- 3-Wire Clock
//    Relay B --PB4--|3   6|--PB1-- 3-Wire Chip Enable
//              GND--|4   5|--PB0-- 3-Wire Data
//                   +-----+
//
// The above pins can be wired to an Arduino UNO for more a convenient
// development flow, see mappings below.
//
// In addition to the pin mapping:
//
// - CFG_ENABLE_RELAYS       - Should the relays be toggled on interrupt.
// - CFG_ENABLE_POWERSAVE    - Use power saving features.
// - CFG_LOGGING             - Send debug logging to Arduino `Serial` out.
// - CFG_ENABLE_BUILT_IN_LED - Use built-in LED for status.
//---------------------------------------------------------------------------

#ifdef ARDUINO_AVR_UNO
// Useful for develpment, replace the ATtiny85 with a UNO board with the
// following wiring.
#define CB_CLOCK_IRQ_PIN   2
#define CB_CLOCK_CE_PIN    4
#define CB_CLOCK_CLK_PIN   5
#define CB_CLOCK_DATA_PIN  6
#define CB_RELAY_A_PIN     8
#define CB_RELAY_B_PIN     9
#define LED_PIN            LED_BUILTIN

#define CFG_ENABLE_RELAYS    true
#define CFG_ENABLE_POWERSAVE false
#define CFG_ENABLE_LOGGING   true
#define CFG_ENABLE_LED       false

void irq_start() {
  attachInterrupt(digitalPinToInterrupt(CB_CLOCK_IRQ_PIN),
                  irq_handler,
                  FALLING);
}

#endif

#ifdef ARDUINO_attiny
#define CB_CLOCK_IRQ_PIN  5
#define CB_CLOCK_CE_PIN   1
#define CB_CLOCK_CLK_PIN  2
#define CB_CLOCK_DATA_PIN 0
#define CB_RELAY_A_PIN    3
#define CB_RELAY_B_PIN    4
#define LED_PIN           4

#define CFG_ENABLE_RELAYS    true
#define CFG_ENABLE_POWERSAVE true
#define CFG_ENABLE_LOGGING   false
#define CFG_ENABLE_LED       false

// Note attachInterrupt() doesn't support PCINT
// interrupts.

void irq_start() {
  // Unmask pin-change IRQ on clock IRQ pin.
  PCMSK = 1u<<CB_CLOCK_IRQ_PIN;

  // Enable pin change IRQ on 0-5.
  GIMSK = 0b00100000;

  // Enable iterrupts
  interrupts();
}

ISR(PCINT0_vect) {
  irq_handler();
}


#endif

#ifndef CB_CLOCK_IRQ_PIN
#error "Sketch doesn't support this board."
#endif

// Timing
#define CB_RELAY_SEC_ON   10
#define CB_RELAY_SEC_GAP  10
