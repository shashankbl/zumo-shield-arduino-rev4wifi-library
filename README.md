# Pololu Zumo Shield Arduino library (Uno R4 WiFi fork)

Fork author: **Shashank Bangalore Lakshman** <br>
Fork date: 2026-04-22 <br>
Upstream version: 2.1.0 (2020-09-11) — [www.pololu.com](https://www.pololu.com/)

This is a fork of the official
[Pololu Zumo Shield Arduino library](https://github.com/pololu/zumo-shield-arduino-library)
that adds support for the **Arduino Uno R4 WiFi** and **Arduino Uno R4 Minima**
(Renesas RA4M1) in addition to the AVR boards supported upstream. See
[Uno R4 support](#uno-r4-support) below for details, caveats, and the small
behavioural differences relative to the AVR port.

## Summary

This is a library for an
[Arduino-compatible controller](https://www.pololu.com/arduino) that
interfaces with the Pololu
[Zumo Shield](https://www.pololu.com/catalog/product/2521),
[Zumo Reflectance Sensor Array](https://www.pololu.com/catalog/product/1419),
[Zumo robot kit](https://www.pololu.com/catalog/product/2509), and
[Zumo robot for Arduino](https://www.pololu.com/catalog/product/2510). It
provides functions to help you program an Arduino-controlled Zumo
robot, and the included example sketches demonstrate how to use them
for more complex tasks.

For more information about the library and examples, please see the
[Zumo Shield user's guide](https://www.pololu.com/docs/0J57).

Please note that this library does NOT work with the Zumo 32U4 Robot, which is a very different product.  The Zumo 32U4 Robot has an integrated Arduino-compatible microcontroller.  If you have the Zumo 32U4 Robot, then you should not use this library and instead refer to the [Zumo 32U4 Robot documentation](https://www.pololu.com/docs/0J63).

## Supported boards

| Board | MCU | Status |
| --- | --- | --- |
| Arduino Uno R3 | ATmega328P | Supported (upstream) |
| Arduino Leonardo / A-Star 32U4 | ATmega32U4 | Supported (upstream) |
| Arduino Uno R4 WiFi | Renesas RA4M1 | **Supported (this fork)** |
| Arduino Uno R4 Minima | Renesas RA4M1 | **Supported (this fork)** |

## Uno R4 support

The Zumo Shield v1.3 is electrically compatible with the Arduino Uno R4: the R4's
Renesas RA4M1 runs 5 V-tolerant I/O on the Uno header, `VIN` accepts the
shield's 7.45 V boost output, and the motor-driver pins (D7–D10), buzzer (D3),
and I2C pins line up with the R3 pinout. The RA4M1 does not have AVR Timer 1
/ Timer 2 / Timer 4, so the motor PWM and buzzer backends are re-implemented
on R4 using the Renesas core's `PwmOut` (GPT-backed) for motors and
Arduino's `tone()` + `millis()` polling for the buzzer.

### Requirements

* **Arduino IDE 2.x** or Arduino CLI.
* **Arduino Renesas Uno Boards** package (Tools → Board → Boards Manager → search for "Uno R4") — provides the RA4M1 core and `PwmOut`.

### Known differences on Uno R4

These are the only user-visible behavioural changes relative to running the
same sketch on an Uno R3 — nothing else in the API changes.

* **Motor PWM carrier is still 20 kHz.** On R4, `ZumoMotors` uses `PwmOut` on
  pins 9 and 10 to drive the DRV8835 at 20 kHz, matching the AVR port (above
  the audible range). Speed range and API (`setSpeeds`, `setLeftSpeed`, etc.)
  are unchanged.
* **Buzzer volume is fixed at ~50 % duty.** R4 uses Arduino's `tone()` for the
  buzzer output, which drives a fixed-duty square wave. The `volume`
  argument to `playFrequency()` / `playNote()` / `play()` is still accepted
  for API compatibility but is effectively ignored on R4 (all non-zero
  volumes play at full volume; volume 0 remains silent).
* **Buzzer frequency resolution is 1 Hz.** AVR supports 0.1 Hz resolution
  below 160 Hz via the `DIV_BY_10` flag; on R4, frequencies are rounded to
  the nearest whole Hz before being handed to `tone()`. Only affects notes
  below ~100 Hz.
* **`PLAY_AUTOMATIC` on R4 requires your loop to poll.** On AVR, note
  sequences advance from a timer ISR in the background. On R4 the
  advancement happens inside `isPlaying()` / `playCheck()` via a
  `millis()` check, so your sketch's `loop()` has to call one of those
  regularly (every ≤ ~50 ms ideally) for notes to advance on time. The
  stock `ZumoBuzzerExample*` sketches already do this; if your own
  sketch blocks the main loop for longer than a note duration, the
  melody will stretch. Note durations themselves are still ms-accurate
  against the clock — only the advance check is polled.

### Pins that must stay off-limits

The R4 WiFi adds a 3.3 V-only Qwiic connector and ESP32-S3 header that the
original Uno R3 does not have. Those headers are **not** 5 V tolerant — don't
wire 5 V Zumo expansion boards (sensor array, etc.) to them. The standard
Uno header pins that the Zumo Shield actually uses are unaffected.

## Getting started

### Hardware

The Zumo Shield for Arduino can be purchased on Pololu's website:
* **[by itself](https://www.pololu.com/catalog/product/2521)**;
* as part of a
  **[Zumo robot kit for Arduino](https://www.pololu.com/catalog/product/2509)**
  that also includes a
  [Zumo chassis](https://www.pololu.com/catalog/product/1418) and a
  stainless steel
  [Zumo blade](https://www.pololu.com/catalog/product/1410); or
* as a fully-assembled
  **[Zumo robot for Arduino](https://www.pololu.com/catalog/product/2510)**
  with [75:1 HP motors](https://www.pololu.com/catalog/product/2361)
  and a
  [reflectance sensor array](https://www.pololu.com/catalog/product/1419)
  installed.

See the [Zumo Shield user's guide](https://www.pololu.com/docs/0J57) for
more details about purchasing and assembling the hardware.


### Software

If you are using version 1.6.2 or later of the
[Arduino software (IDE)](https://www.arduino.cc/en/Main/Software), you can use
the Library Manager to install this library:

1. In the Arduino IDE, open the "Sketch" menu, select "Include Library", then
   "Manage Libraries...".
2. Search for "ZumoShield".
3. Click the ZumoShield entry in the list.
4. Click "Install".

If this does not work, you can manually install the library:

1. Download the ZIP file for the
   [latest tag from GitHub](https://github.com/pololu/zumo-shield-arduino-library/tags)
   and decompress it.
2. Rename the folder "zumo-shield-arduino-library-xxxx" to "ZumoShield".
3. Drag the "ZumoShield" folder into the "libraries" directory inside your
   Arduino sketchbook directory. You can view your sketchbook location by
   opening the "File" menu and selecting "Preferences" in the Arduino IDE. If
   there is not already a "libraries" folder in that location, you should make
   the folder yourself.
4. After installing the library, restart the Arduino IDE.

## Examples

Several example sketches are available that show how to use the
library. You can access them from the Arduino IDE by opening the
"File" menu, selecting "Examples", and then selecting "ZumoShield". If
you cannot find these examples, the library was probably installed
incorrectly and you should retry the installation instructions above.

The Example sketches section of
the [Zumo Shield user's guide](https://www.pololu.com/docs/0J57)
describes some of these examples in more detail.

**Fork-added example:** `LineFollowerInverted` mirrors the stock
`LineFollower` but tracks a **white line on a dark surface** by
passing `white_line = 1` to `QTRSensors::readLine()`. The calibration
sweep, PID loop, and motor control are otherwise identical — useful
if your arena uses light tape on dark stock rather than the default
dark-on-light.

## Classes

The main classes provided by the library are listed below:

* ZumoBuzzer
* ZumoIMU
* ZumoMotors
* ZumoReflectanceSensorArray

## Component libraries

This library also includes copies of several other Arduino libraries inside it, which are used to help implement the classes and functions above.

* [PololuBuzzer](https://github.com/pololu/pololu-buzzer-arduino)
* [Pushbutton](https://github.com/pololu/pushbutton-arduino)
* [QTRSensors](https://github.com/pololu/qtr-sensors-arduino)

Additionally, the [LSM303](https://github.com/pololu/lsm303-arduino) and [L3G](https://github.com/pololu/l3g-arduino) libraries are included for backward compatibility with older versions of the ZumoShield library, but we recommend using the ZumoIMU class to interface with the inertial sensors instead.

You can use these libraries in your sketch automatically without any extra installation steps and without needing to add any extra `#include` lines to your sketch.

You should avoid adding extra `#include` lines such as `#include <Pushbutton.h>` because then the Arduino IDE might try to use the standalone Pushbutton library (if you previously installed it), and it would conflict with the copy of the Pushbutton code included in this library.  The only `#include` lines needed to access all features of this library are:

~~~{.cpp}
#include <Wire.h>
#include <ZumoShield.h>
~~~

## Documentation

For complete documentation, see
https://pololu.github.io/zumo-shield-arduino-library. If you are
already on that page, then click on the links in the "Classes and
functions" section above.

## Version history

* **Fork — Uno R4 WiFi support (2026-04-22):** Added Arduino Uno R4 WiFi / Uno R4 Minima (Renesas RA4M1) support. `ZumoMotors` drives pins 9/10 through `PwmOut` at 20 kHz; `PololuBuzzer` uses Arduino's `tone(pin, freq, duration)` for the audio output and `millis()` polling inside `isPlaying()` / `playCheck()` for note-sequence advancement. AVR (ATmega328P / ATmega32U4) paths are unchanged. Known limitations on R4: buzzer volume and sub-Hz frequency resolution are not available, and `PLAY_AUTOMATIC` requires the sketch's `loop()` to call `isPlaying()` / `playCheck()` regularly rather than advancing notes from an ISR.
* 2.1.0 (2020-09-11): Added a ZumoIMU class that abstracts some details of the inertial sensors and supports different IMU types. The examples have been updated to use this class, and a few new examples have been added.
* 2.0.0 (2018-03-15):
    * Forked [https://github.com/pololu/zumo-shield](https://github.com/pololu/zumo-shield)
    * Consolidated sub-libraries into one library called ZumoShield.
    * Added LSM303 and L3G as component libraries
    * Added Travis CI testing.
    * Updated library to work with the Arduino Library Manager.
* 1.2.3 (2013-11-27): Updated examples to work with LSM303 library version 2.0.0.
* 1.2.2 (2013-10-08): Added SumoCollisionDetect example.
* 1.2.1 (2013-07-19): Added LineFollower example.
* 1.2.0 (2013-06-03): Added ZumoExamples dummy library containing example projects.
* 1.1.3 (2013-05-14): Added CompassExample.
* 1.1.2 (2013-04-18): Pulled in QTRSensors version 2.1.0.
* 1.1.1 (2013-04-17): Added ZumoReflectanceSensorArray constructors with parameters to call init() with and pulled fix from qtr-sensors-arduino 2.0.2.
* 1.1.0 (2013-01-07): Added ZumoReflectanceSensorArray and QTRSensors libraries.
* 1.0.0 (2012-11-09): Original release.
