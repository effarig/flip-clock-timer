# Flip Clock Timer
Little project to fix my clock.

## Problem
The flip clock consists of a fairly standard looking timekeeping clock part
and a flip mechanism to show AM/PM, the day or month, weekday and month. The
timekeeping clock part is a self-contained clock module which should trigger
the date flip mechanism twice a day, approximately at noon and midnight.

Unfortunately the mechanism in the clock to trigger the date flip mechanism
is unreliable. It seems to work intermittently, sometimes not at all for
weeks at a time.

## The Trigger
The date part is connected to the clock part via a three pin single-in-line
connector:
- Pin 1: Blue, `A`
- Pin 2: Red, `Common`
- Pin 3: Yellow, `B`

Can trigger incrementing the date as follows:
1. Short `A` to `Common`.
2. Wait a few seconds.
3. Disconnect `A` and `Common`.
4. Wait a few seconds.
5. Short `B` and `Common`.
6. Wait a few seconds.
7. Disconnect `B` and `Common`.

## Aim
Create device to simulate trigger by the clock part:
- Share clock's existing 3V battery.
- 2V-3V operation.
- Low drain on battery.
- No irreversable changes to flip clock.

## Operation
Once connected to the clock, it needs to be turned on at around noon or
midnight. On startup ATtiny configures DC1305 setting alarms to raise
an IRQ at noon and midnight. It then enters a low power wait for interrupt
state. When the RTC alarm IRQ occurs, the ATtiny wakes up, clears the RTC
IRQ, turns on relay A for 10 seconds, 10 seconds later it turns on relay B
for 10 seconds and then goes back to sleep.

## Files
- `*.ino` Arduino sketch, compatible with Uno for development and ATtiny85 for embedding in the clock. See [`flip-clock-timer.ino`](./flip-clock-timer.ino).
- `fuses.txt` ATtiny fuse settings for programming with a TL866ii+
- `librepcb` Schematic as [LibrePCB](https://librepcb.org/) project.

## Notes
### DC1305 RTC
- Needs external crystal.
- Can configure two repeating alarms.
- Using 3-wire SPI like bus.
- Powered by battery only: `VCC1=0V`, `VBATT=0V`, `VCC2=3V`.
- Low power; when `CE=0V` timekeeping current is at most 1.0μA, depending on voltage.
- Operates down to 2V.

### ATtiny85
- Cheap, low power, just enough I/O pins.
- Can develop the software using an Uno for convenience and serial debugging.
- Disable ADC, and bandgap reference during sleep to get the power-down current to under 1μA.
- Need to use PB5 `RESET` for I/O:
  - RSTDISBL fuse enabled.
  - As a result, can't program using ISP via an Uno, use TI866ii instead.
  - Note PB5 is a weak output, it struggles saturate relay driver transistor; use as `nIRQ` input.

### Relays
- Relays draw about 150mA at 3V, 100mA at 2V.
- Each driven by left over ZTX 302s (now obsolete).
- Each relay on for 10s every 12 hours.

## Resources
- [ATtiny datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf).
- [DC1305 datasheet](https://datasheets.maximintegrated.com/en/ds/DS1305.pdf).
- [Power saving on AVR microcontrolers](http://www.gammon.com.au/power).
- [Using TI866 woth Arduino IDE](https://forum.arduino.cc/t/minipro-tl866cs-universal-programmer-working-with-arduino-ide/373335).
