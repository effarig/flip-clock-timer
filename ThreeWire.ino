//---------------------------------------------------------------------------
// Crude bit banged 3-wire bus implementation.
//---------------------------------------------------------------------------
// Very slow as its full of millisecond sleeps, however for this application
// that's not a problem.
//---------------------------------------------------------------------------
class ThreeWire
{
public:
  ThreeWire() :
    m_enable_pin(-1),
    m_clock_pin(-1),
    m_data_pin(-1)
  {}

  ThreeWire(int enable_pin, int clock_pin, int data_pin) :
    m_enable_pin(enable_pin),
    m_clock_pin(clock_pin),
    m_data_pin(data_pin)
  {
    pinMode(m_enable_pin, OUTPUT);
    pinMode(m_clock_pin , OUTPUT);
    pinMode(m_data_pin  , INPUT);

    digitalWrite(m_enable_pin, LOW);
    digitalWrite(m_clock_pin, LOW);
  }

  void write_block(uint8_t address, uint8_t const* blk, int bytes);
  void read_block(uint8_t address, uint8_t* blk, int bytes);

  void write_byte(uint8_t const address, uint8_t const value);
  uint8_t read_byte(uint8_t const address);

private:
  int m_enable_pin;
  int m_clock_pin;
  int m_data_pin;

  void put_byte(uint8_t value);
  uint8_t get_byte();
};

void ThreeWire::put_byte(uint8_t value) {

  for (unsigned bit = 0; bit < 8; ++bit) {
    digitalWrite(m_clock_pin, LOW);
    delay(1);

    pinMode(m_data_pin, OUTPUT);
    digitalWrite(m_data_pin, (value >> bit) & 1);
    delay(1);

    digitalWrite(m_clock_pin, HIGH);
    delay(1);

    pinMode(m_data_pin, INPUT);
  }
}

uint8_t ThreeWire::get_byte() {
  uint8_t out = 0;
  for (unsigned bit = 0; bit < 8; ++bit) {
    digitalWrite(m_clock_pin, HIGH);
    delay(1);

    digitalWrite(m_clock_pin, LOW);
    delay(1);

    if (digitalRead(m_data_pin)) out = out | (1u << bit);
    delay(1);
  }
  return out;
}


void ThreeWire::write_block(uint8_t address, uint8_t const* blk, int bytes) {
  // Ensure data line not driving bus.
  pinMode(m_data_pin, INPUT);

  // Clock low to start writing address.
  digitalWrite(m_clock_pin, LOW);
  delay(1);

  // Enable interface
  digitalWrite(m_enable_pin, HIGH);
  delay(1);

  // Send command and data.
  put_byte(address | (1u << 7));
  for (int i = 0; i < bytes; ++i) {
    put_byte(blk[i]);
  }

  // Disable interface
  digitalWrite(m_enable_pin, LOW);
  delay(1);

  // Ensure data line not driving bus.
  pinMode(m_data_pin, INPUT);
  delay(1);
}

void ThreeWire::read_block(uint8_t address, uint8_t* blk, int bytes) {
  // Ensure data line not driving bus.
  pinMode(m_data_pin, INPUT);

  // Clock low to start writing address.
  digitalWrite(m_clock_pin, LOW);
  delay(1);

  // Enable interface
  digitalWrite(m_enable_pin, HIGH);
  delay(1);

  // Send command and data.
  put_byte(address);

  while (bytes-- > 0) {
    *blk++ = get_byte();
  }

  // Disable interface
  digitalWrite(m_enable_pin, LOW);
  delay(1);

  // Ensure data line not driving bus.
  pinMode(m_data_pin, INPUT);
  delay(1);
}

void ThreeWire::write_byte(uint8_t const address, uint8_t const value) {
  write_block(address, &value, 1);
}

uint8_t ThreeWire::read_byte(uint8_t const address) {
  uint8_t out = 0;
  read_block(address, &out, 1);
  return out;
}
