# Flip Clock Timer
Little project to fix my clock.

## Problem
The Flip Clock consists of a fairly standard looking timekeeping clock part
and a flip mechanism to show AM/PM, the day or month, weekday and month. The
timekeeping clock part is a self-contained clock module which should trigger
the flip mechanism twice a day, approximately at noon and midnight.

Unfortunately the trigger is unreliable. It seems to work for a few days
or weeks, then stop working, then start working again.

## The Trigger
The date part is connected to the clock part via a three pin single-in-line
connector:
- Pin 1: Blue, `A`
- Pin 2: Red, `Common`
- Pin 3: Yellow, `B`

Every twelve hours the trigger operates as follows:
1. Short `A` to `Common`.
2. Wait.
3. Disconnect `A` and `Common`.
4. Wait.
5. Short `B` and `Common`.
6. Wait.
7. Disconnect `B` and `Common`.

## Aim
Create device to simulate trigger by the clock part:
- Share clock's existing 3V battery.
- 2V-3V operation.
- Low drain on battery.
- No irreversable changes to flip clock.

## Files
- `*.ino` Arduino sketch, compatible with Uno and ATtiny85, see [`flip-clock-timer.ino`](./flip-clock-timer.ino).
- `fuses.txt` ATtiny fuse settings for TL866ii+

## Notes
### DC1305 RTC
- Relatively expensive; limited avalability of through-the-hole alternatives at the time.
- Has two alarms; low power.
- No I2C, using 3-wire SPI like bus.
- Powered by battery only: `VCC1=0V`, `VBATT=0V`, `VCC2=3V`.
- When `CE=0V` timekeeping current is at most 1.0μA, depending on voltage.

### ATtiny85
- Cheep, low power, just enough I/O pins.
- Can develop the software using an Uno for convenience and serial debugging.
- Disable ADC, and bandgap reference during sleep to get the power down current to approximately 0.4μA.
- Need to use PB5 for I/O:
  - RSTDISBL fuse enabled.
  - Can't use ISP via Uno, use TI866 instead.
  - Note PB5 (RESET) struggles saturate relay driver transistor; use as nIRQ input.

### Relays
- Relays draw about 150mA at 3V, 100mA at 2V.
- Each driven by a ZTX 302 from the spares box; now obsolete.
- Each relay on for 10s every 12 hours.

## Resources
- [ATtiny datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf).
- [DC1305 datasheet](https://datasheets.maximintegrated.com/en/ds/DS1305.pdf).
- [Power saving on AVR microcontrolers](http://www.gammon.com.au/power).
- [Using TI866 woth Arduino IDE](https://forum.arduino.cc/t/minipro-tl866cs-universal-programmer-working-with-arduino-ide/373335).
