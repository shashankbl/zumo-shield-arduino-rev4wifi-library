# Hardware testing guide — Uno R4 WiFi on the Pololu Zumo Shield v1.3

This document walks through bringing up the `feature/uno-r4-wifi-timers`
branch on a real Zumo robot with an Arduino Uno R4 WiFi plugged into the
shield. Work through the sections in order — each test isolates one
subsystem so that if something fails you know exactly which subsystem
owns it.

## 0. What you need

**Hardware**
- Pololu Zumo Shield v1.3 (assembled on a Zumo chassis with 75:1 HP motors
  and 4× AA batteries, or equivalent).
- Arduino Uno R4 WiFi (or R4 Minima — same MCU).
- USB-C cable for programming.
- Optional: Pololu Zumo Reflectance Sensor Array, if you plan to test
  the line-following subsystem.

**Software**
- Arduino IDE 2.x (or Arduino CLI ≥ 0.35).
- **Arduino Renesas Uno Boards** package installed (Tools → Board →
  Boards Manager → search "Uno R4" → install).
- This fork cloned locally:
  ```
  git clone https://github.com/shashankbl/zumo-shield-arduino-rev4wifi-library.git
  cd zumo-shield-arduino-rev4wifi-library
  git checkout feature/uno-r4-wifi-timers
  ```

**Safety checks before powering on**
- Battery polarity in the Zumo battery holder is correct.
- The Arduino is seated fully on the shield headers with no bent pins.
- Nothing is touching the motor terminals or exposed leads on the
  underside of the shield.
- Put the Zumo on a book or small box so the wheels spin free during
  early motor tests — the first motor sketch will move the robot.

## 1. Install the library in the Arduino IDE

Arduino needs to see this library to compile the examples that
`#include <ZumoShield.h>`.

1. Close the Arduino IDE if it's open.
2. Symlink (or copy) the cloned repo into your Arduino libraries folder:
   ```
   ln -s "$(pwd)" ~/Arduino/libraries/ZumoShield
   ```
   (On Windows, copy the folder to `Documents\Arduino\libraries\ZumoShield`.
   On macOS, it's `~/Documents/Arduino/libraries/ZumoShield`.)
3. Re-open the Arduino IDE. Confirm it loaded:
   `File → Examples → ZumoShield` should now list
   `ZumoMotorExample`, `ZumoBuzzerExample`, etc.

**If the examples don't appear:** check that the library folder is named
exactly `ZumoShield` (not `zumo-shield-arduino-rev4wifi-library`) and
that `library.properties` is at its top level.

## 2. Select the board

1. Plug the Uno R4 WiFi into USB.
2. `Tools → Board → Arduino Renesas Uno Boards → Arduino Uno R4 WiFi`.
3. `Tools → Port → <your USB port>` (typically `/dev/ttyACM0` on Linux,
   `COMn` on Windows).
4. Compile a trivial sketch first (e.g. `File → Examples → 01.Basics →
   Blink`) and upload it. **Don't proceed until this baseline works** —
   if it fails, the toolchain is broken and Zumo-specific tests will
   just add noise.

## 3. Test motors in isolation — `ZumoMotorExample`

This is the most important first test because it exercises the new
`PwmOut`-based 20 kHz PWM backend on pins 9 and 10.

1. **Prop the wheels off the ground.**
2. `File → Examples → ZumoShield → ZumoMotorExample`.
3. Verify the sketch compiles. If you see any error mentioning
   `TCCR1A`, `OCR1A`, `OCR1B`, or `ICR1`, you're compiling the AVR path
   by mistake — recheck the board selection.
4. Upload. Open the Serial Monitor at 9600 baud.

**Expected behaviour**
- User-button press (Zumo pushbutton, D12) starts a demo: forward, turn,
  backward, reverse-turn — each for a short interval.
- Both wheels spin smoothly at matched speeds.
- **No audible whine from the motors.** If you hear a clear ~500 Hz tone
  when the motors run, the `PwmOut` 20 kHz path didn't activate and the
  code fell through to `analogWrite()`'s default — check that
  `ARDUINO_UNOR4_WIFI` is being defined during the build.

**Direction check**
- `setSpeeds(200, 200)` → forward (both wheels spin "outward"
  at the bottom).
- `setSpeeds(-200, -200)` → backward.
- If one wheel goes the wrong way, use `flipLeftMotor(true)` or
  `flipRightMotor(true)` in `setup()` — the motor wiring on some Zumo
  kits is mirrored.

## 4. Test the buzzer — `ZumoBuzzerExample`

Exercises the R4 backend: `tone(pin, freq, duration)` for the audio
plus `millis()` polling inside `isPlaying()` / `playCheck()` for
note-sequence advancement.

1. `File → Examples → ZumoShield → ZumoBuzzerExample`.
2. Upload. The Super Mario theme melody should play through on its own.
3. Press the Zumo pushbutton (D12) to silence / restart the melody.

**Expected behaviour**
- Melody plays note-by-note at the programmed rhythm — no stuttering,
  no hung continuous tones.
- Notes advance from inside `isPlaying()`, which the example polls in
  its `loop()`. **On R4, if you write your own sketch that blocks the
  main loop for longer than a note duration, the melody will stretch**
  — this is a documented behavioural difference from AVR (see README).

**Known differences vs the Uno R3** (documented in [README.md](README.md)
under "Uno R4 support"):
- Volume is fixed at ~50 % duty — passing `volume = 5` vs `volume = 15`
  produces the same loudness on R4. This is expected, not a bug.
- Frequencies below ~100 Hz are rounded to whole Hz; a 41.2 Hz note
  plays at 41 Hz.

**If nothing plays at all:**
- **Check the Zumo Shield's buzzer jumper first.** Most Zumo Shield
  versions have a small jumper near the buzzer can that bridges D3 to
  the buzzer's control transistor. If the jumper is missing or on the
  wrong pins, the buzzer is electrically disconnected from D3 and
  nothing your code does will produce sound. Photos and location:
  [Zumo Shield user's guide §5](https://www.pololu.com/docs/0J57).
  This is the #1 cause of a silent buzzer; rule it out before
  suspecting the library.
- **Verify the buzzer hardware with raw `tone()`.** Upload the
  four-line smoke test below into a blank sketch — if it doesn't play
  a clean 2-second beep, the problem is lower than this library
  (jumper, wiring, or dead buzzer):
  ```cpp
  void setup() { tone(3, 1000); delay(2000); noTone(3); }
  void loop()  {}
  ```
- **Confirm battery power.** The buzzer circuit is driven from VBAT
  on most Zumo Shield versions, not USB 5 V. Switch the Zumo battery
  pack on and verify fresh cells.
- Confirm the compiler actually picked the R4 branch by adding a
  temporary `#warning "R4 path active"` at the top of the R4 block in
  [PololuBuzzer.cpp:14](PololuBuzzer.cpp#L14) and looking for it in the
  compile log.

## 5. Test the pushbutton — `PushbuttonExample`

Sanity check for `digitalRead()` on D12 and the internal pull-up on R4.
The stock example uses only the onboard LED for feedback — no serial
output.

1. Upload `PushbuttonExample`.
2. Press and release the Zumo user button (D12). The onboard LED (D13)
   should blink once per full press-and-release cycle.
3. The sketch exercises three detection paths in sequence — `isPressed()`
   with manual debounce, `waitForButton()`, and `getSingleDebouncedRelease()`.
   Press the button three times to cycle through all three; each press
   should produce an LED blink before the sketch advances to the next
   method.

This should work with zero code changes — the Pushbutton class uses only
standard Arduino APIs.

**Want serial feedback too?** Add these four edits to
[PushbuttonExample.ino](examples/PushbuttonExample/PushbuttonExample.ino)
(only needed if the LED-only output isn't enough for your bring-up):

```cpp
void setup()
{
  Serial.begin(9600);            // add this
  pinMode(LED_PIN, OUTPUT);
}
```

Then add a `Serial.println("pressed (methodN)");` after each
`digitalWrite(LED_PIN, HIGH)` line. Open the Serial Monitor at
**9600 baud, line ending: "Newline"** and you'll see one line per
press.

## 6. Test the inertial sensors — `InertialSensors`

Exercises I2C (`Wire`) and the `ZumoIMU` class.

1. Upload `InertialSensors`.
2. Open Serial Monitor at 9600 baud.
3. Expect a continuous stream of accelerometer, gyro, and magnetometer
   readings.

**Expected behaviour**
- With the Zumo stationary on a flat surface:
  - Accelerometer Z ≈ 1 g (~16000 on an LSM6DS33 in the default range),
    X and Y near zero.
  - Gyro values fluctuate around zero (a few hundred counts of noise is
    normal).
- Rotate the robot by hand — gyro values on the rotated axis should
  spike, then return to noise when stationary again.

**If values are all zero or all -1:** the I2C bus isn't talking. On
R4 WiFi, SDA/SCL are at the same physical header positions as the R3,
so wiring isn't the issue — check `Tools → Board` is set to R4 WiFi
and not R4 Minima if you have a WiFi board (the pin mapping differs on
the secondary headers but the I2C pins are identical for the Zumo).

## 7. Test the reflectance sensor array — `LineSensorTest`

*(Skip this section if you don't have the Zumo Reflectance Sensor Array
attached.)*

1. Upload `LineSensorTest`.
2. Serial Monitor at 9600 baud.
3. Hold the Zumo over a white surface → all 6 sensor readings should be
   low (reflective → short pulse).
4. Slide a strip of black electrical tape under the sensors → readings
   over the tape should spike to ~2000 (the `timeout` default).

No code change was needed for the sensor array on R4, but this confirms
that the hardcoded pin array `{4, A3, 11, A0, A2, 5}` in
[ZumoReflectanceSensorArray.h:155](ZumoReflectanceSensorArray.h#L155)
maps correctly to the R4 WiFi's header pins.

## 8. Full-stack integration test — `LineFollower` or `SumoCollisionDetect`

Pick one to prove motors + buzzer + sensors all work together.

- **LineFollower**: needs a track (black tape on white). Exercises
  motors, reflectance array, and the calibration flow.
- **SumoCollisionDetect**: uses the IMU to detect impacts. Exercises
  motors + IMU + buzzer.

Upload, place the robot on the required surface, press the user button,
and confirm end-to-end behaviour matches what the example's README or
comment header describes.

## 9. Regression test — verify you didn't break the AVR path

If you also have an Uno R3 lying around:

1. Swap the R4 WiFi off the shield and install the R3 (power off first).
2. `Tools → Board → Arduino AVR Boards → Arduino Uno`.
3. Recompile and re-upload each example from sections 3–8.
4. Everything should still work exactly as it did on upstream 2.1.0.

The AVR code paths in this fork are gated by `__AVR__` / `__AVR_ATmega328P__`
/ `__AVR_ATmega32U4__` and are byte-identical to upstream. If an AVR
regression appears, it's a bug in this fork and should be filed as
an issue.

## 10. Reporting results

When you report test results (or file a bug), please include:

- Arduino IDE version and Renesas Uno Boards package version.
- Output of `git -C <repo> rev-parse HEAD` so we know the exact commit.
- Which example(s) failed, with the full serial output.
- Expected vs observed behaviour.
- Whether the same example works on an Uno R3 (to rule out a shield
  wiring issue vs a port regression).

## Quick reference: what each subsystem relies on

| Subsystem | R4 backend | Files touched by the port |
| --- | --- | --- |
| Motor PWM (D9, D10) | `PwmOut` @ 20 kHz | [ZumoMotors.cpp](ZumoMotors.cpp) |
| Motor direction (D7, D8) | `digitalWrite` | unchanged |
| Buzzer (D3) | `tone(pin, freq, dur)` auto-stop + `millis()` polling in `isPlaying()` | [PololuBuzzer.cpp](PololuBuzzer.cpp), [PololuBuzzer.h](PololuBuzzer.h) |
| Pushbutton (D12) | `digitalRead` w/ pull-up | unchanged |
| Reflectance sensors (4, A3, 11, A0, A2, 5) | `micros()` pulse timing | unchanged |
| IMU (I2C on SDA/SCL) | `Wire` | unchanged |
