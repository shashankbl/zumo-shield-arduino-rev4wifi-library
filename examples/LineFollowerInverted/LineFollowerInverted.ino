/*
 * Line-following for the Pololu Zumo Robot — INVERTED track (white line
 * on a black/dark surface).
 *
 * This is a companion to LineFollower.ino, which expects a black line on
 * a white surface. The only meaningful difference is the `white_line`
 * argument passed to `readLine()` — the PID loop, calibration sweep, and
 * motor control are identical. `readLine()` internally inverts each
 * calibrated reading when `white_line == 1`, so downstream math treats
 * "over the line" the same way regardless of track colour.
 *
 * Calibration tip: during the 1.6 s calibration sweep after the first
 * button press, the sensors must see BOTH the bright white tape and the
 * dark surrounding surface. The sweep is the same as for the standard
 * LineFollower; no track-specific adjustments needed.
 *
 * Works on Zumo Shield v1.3 with Arduino Uno R3 and Arduino Uno R4 WiFi.
 *
 * Track suggestions:
 *   - White electrical tape (or paper tape) on black matte cardstock /
 *     poster board / duct tape.
 *   - Keep the line roughly centred on the reflectance array; 6" (15 cm)
 *     radius curves work well at the default PID tuning below.
 */

#include <Wire.h>
#include <ZumoShield.h>

ZumoBuzzer buzzer;
ZumoReflectanceSensorArray reflectanceSensors;
ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);
int lastError = 0;

// Maximum motor speed (400 = full). Lower this for smoother, slower runs.
const int MAX_SPEED = 400;

// Pass to readLine() to flip its interpretation: 1 = white line on dark
// surface; 0 = black line on white surface (the default in LineFollower.ino).
const unsigned char WHITE_LINE = 1;

void setup()
{
  // Welcome chirp.
  buzzer.play(">g32>>c32");

  reflectanceSensors.init();

  // Wait for the user button to be pressed and released before starting
  // calibration — gives you time to position the robot.
  button.waitForButton();

  // LED 13 on = calibration in progress.
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // Rotate in place to sweep the sensor array across the line, taking
  // calibration samples continuously. 80 iterations × 20 ms = 1.6 s total.
  delay(1000);
  for (int i = 0; i < 80; i++)
  {
    if ((i > 10 && i <= 30) || (i > 50 && i <= 70))
      motors.setSpeeds(-200, 200);
    else
      motors.setSpeeds(200, -200);
    reflectanceSensors.calibrate();
    delay(20);
  }
  motors.setSpeeds(0, 0);

  digitalWrite(13, LOW);
  buzzer.play(">g32>>c32");

  // Wait for the second button press to begin line-following.
  button.waitForButton();

  // Short confirmation jingle — blocking so the motors don't start mid-note.
  buzzer.play("L16 cdegreg4");
  while (buzzer.isPlaying());
}

void loop()
{
  unsigned int sensors[6];

  // readLine() returns a weighted-average position estimate in the range
  // 0..5000 (for a 6-sensor array). The WHITE_LINE argument inverts the
  // per-sensor values so the estimator treats "more reflective" as "on
  // the line" — matching the physical layout of a white line on dark.
  int position = reflectanceSensors.readLine(sensors, QTR_EMITTERS_ON, WHITE_LINE);

  // Distance from the array centre (2500 = directly over the line).
  int error = position - 2500;

  // Simple PD controller: proportional + derivative terms. No integral.
  // Constants copied from the standard LineFollower — retune for your
  // chassis / motor combo if the robot wobbles or overshoots.
  int speedDifference = error / 4 + 6 * (error - lastError);
  lastError = error;

  int m1Speed = MAX_SPEED + speedDifference;
  int m2Speed = MAX_SPEED - speedDifference;

  // Clamp: one motor at MAX_SPEED, the other at (MAX_SPEED - |diff|),
  // never reversing. Allow negative values if you want pivot-style turns.
  if (m1Speed < 0)        m1Speed = 0;
  if (m2Speed < 0)        m2Speed = 0;
  if (m1Speed > MAX_SPEED) m1Speed = MAX_SPEED;
  if (m2Speed > MAX_SPEED) m2Speed = MAX_SPEED;

  motors.setSpeeds(m1Speed, m2Speed);
}
