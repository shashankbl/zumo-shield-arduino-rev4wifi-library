#include "ZumoMotors.h"

#define PWM_L 10
#define PWM_R 9
#define DIR_L 8
#define DIR_R 7

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined (__AVR_ATmega32U4__)
  #define USE_20KHZ_PWM
#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
  // On the Renesas RA4M1 we drive the motors through the PwmOut class (GPT-backed)
  // so we can pick our own 20 kHz frequency instead of the ~490 Hz default from
  // analogWrite() — eliminating the audible whine from the DRV8835.
  #define USE_ZM_R4_PWMOUT
  #include "pwm.h"
  static PwmOut pwmL(PWM_L);
  static PwmOut pwmR(PWM_R);
#endif

static boolean flipLeft = false;
static boolean flipRight = false;

// constructor (doesn't do anything)
ZumoMotors::ZumoMotors()
{
}

// initialize timer1 to generate the proper PWM outputs to the motor drivers
void ZumoMotors::init2()
{
  pinMode(DIR_L, OUTPUT);
  pinMode(DIR_R, OUTPUT);

#ifdef USE_20KHZ_PWM
  pinMode(PWM_L, OUTPUT);
  pinMode(PWM_R, OUTPUT);
  // Timer 1 configuration
  // prescaler: clockI/O / 1
  // outputs enabled
  // phase-correct PWM
  // top of 400
  //
  // PWM frequency calculation
  // 16MHz / 1 (prescaler) / 2 (phase-correct) / 400 (top) = 20kHz
  TCCR1A = 0b10100000;
  TCCR1B = 0b00010001;
  ICR1 = 400;
#elif defined(USE_ZM_R4_PWMOUT)
  // PwmOut owns the pin direction; no pinMode() needed for PWM_L / PWM_R.
  // Start at non-zero duty so the GPT channel actually enables its output —
  // some Renesas core versions will leave the pin in GPIO/high-Z if begin()
  // is called with duty_perc == 0.0f, and subsequent pulse_perc() calls
  // then silently do nothing. Zero the duty immediately afterward so the
  // motor doesn't twitch during init (DIR pins default LOW, so any brief
  // pulse is forward-biased and <1 ms long).
  pwmL.begin(20000.0f, 50.0f);
  pwmR.begin(20000.0f, 50.0f);
  pwmL.pulse_perc(0.0f);
  pwmR.pulse_perc(0.0f);
#else
  pinMode(PWM_L, OUTPUT);
  pinMode(PWM_R, OUTPUT);
#endif
}

// enable/disable flipping of left motor
void ZumoMotors::flipLeftMotor(boolean flip)
{
  flipLeft = flip;
}

// enable/disable flipping of right motor
void ZumoMotors::flipRightMotor(boolean flip)
{
  flipRight = flip;
}

// set speed for left motor; speed is a number between -400 and 400
void ZumoMotors::setLeftSpeed(int speed)
{
  init(); // initialize if necessary
    
  boolean reverse = 0;
  
  if (speed < 0)
  {
    speed = -speed; // make speed a positive quantity
    reverse = 1;    // preserve the direction
  }
  if (speed > 400)  // Max 
    speed = 400;
    
#ifdef USE_20KHZ_PWM
  OCR1B = speed;
#elif defined(USE_ZM_R4_PWMOUT)
  // speed range is 0..400; PwmOut::pulse_perc expects 0.0..100.0
  pwmL.pulse_perc(speed * 0.25f);
#else
  analogWrite(PWM_L, speed * 51 / 80); // default to using analogWrite, mapping 400 to 255
#endif

  if (reverse ^ flipLeft) // flip if speed was negative or flipLeft setting is active, but not both
    digitalWrite(DIR_L, HIGH);
  else
    digitalWrite(DIR_L, LOW);
}

// set speed for right motor; speed is a number between -400 and 400
void ZumoMotors::setRightSpeed(int speed)
{
  init(); // initialize if necessary
    
  boolean reverse = 0;
  
  if (speed < 0)
  {
    speed = -speed;  // Make speed a positive quantity
    reverse = 1;  // Preserve the direction
  }
  if (speed > 400)  // Max PWM dutycycle
    speed = 400;
    
#ifdef USE_20KHZ_PWM
  OCR1A = speed;
#elif defined(USE_ZM_R4_PWMOUT)
  pwmR.pulse_perc(speed * 0.25f);
#else
  analogWrite(PWM_R, speed * 51 / 80); // default to using analogWrite, mapping 400 to 255
#endif

  if (reverse ^ flipRight) // flip if speed was negative or flipRight setting is active, but not both
    digitalWrite(DIR_R, HIGH);
  else
    digitalWrite(DIR_R, LOW);
}

// set speed for both motors
void ZumoMotors::setSpeeds(int leftSpeed, int rightSpeed)
{
  setLeftSpeed(leftSpeed);
  setRightSpeed(rightSpeed);
}