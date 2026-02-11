# Birthday Countdown Clock

An Arduino-based countdown clock that displays the time and counts down days until birthdays and Christmas on an 8x32 LED matrix.

## Hardware

- **Arduino Nano** (ATmega328P)
- **8x32 LED Matrix** — 4 daisy-chained MAX7219 8x8 modules (FC16_HW type)
- **DS3231 RTC** — real-time clock module (I2C)
- **3 push buttons** — with internal pull-up resistors

### Wiring

| Component | Arduino Pin |
|-----------|-------------|
| LED Matrix DIN | D11 (MOSI) |
| LED Matrix CLK | D13 (SCK) |
| LED Matrix CS | D10 |
| Button 1 (V's birthday) | D2 |
| Button 2 (B's birthday) | D3 |
| Button 3 (Christmas) | D4 |
| DS3231 SDA | A4 |
| DS3231 SCL | A5 |

## Features

### Clock Mode (default)

Displays the current time in `HH:MM` format at low brightness. Updates once per minute.

### Countdown Mode

Each button triggers a countdown to a specific date:

- **Button 1** — V's birthday (May 30)
- **Button 2** — B's birthday (July 28)
- **Button 3** — Christmas (December 24)

The countdown shows the person's initial (or an icon for Christmas), an animated icon (cake, present, heart, tree, star), and the number of remaining days. Icons rotate every 3 seconds.

### Dot Display Mode

A visual representation of remaining days where each lit LED on the matrix represents one day. Dots fill left-to-right, bottom-to-top (row by row).

When the remaining days exceed 256 (the matrix capacity of 8x32), the display cycles between full screens (all 256 LEDs lit) and the remainder, switching every 3 seconds.

### Button Cycle

Each button cycles through three states:

1. **1st press** — text countdown with animated icons
2. **2nd press** — dot display (one LED per remaining day)
3. **3rd press** — back to clock

The display automatically returns to clock mode after 30 seconds of inactivity.

## Libraries

- [MD_Parola](https://github.com/MajicDesigns/MD_Parola) — LED matrix text and animation
- [MD_MAX72XX](https://github.com/MajicDesigns/MD_MAX72XX) — MAX7219 LED driver
- [RTClib](https://github.com/adafruit/RTClib) — DS3231 real-time clock

## Tools

- `led-matrix.html` — interactive 8x32 LED matrix simulator for designing custom character patterns

## Building

Install the [Arduino CLI](https://arduino.github.io/arduino-cli/) and required libraries:

```sh
arduino-cli lib install MD_Parola MD_MAX72XX RTClib
```

Compile:

```sh
arduino-cli compile --fqbn arduino:avr:nano birthday_countdown_clock.ino
```

Upload:

```sh
arduino-cli upload --fqbn arduino:avr:nano -p /dev/ttyUSB0 birthday_countdown_clock.ino
```
