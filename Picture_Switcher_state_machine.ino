/*
  This sketch makes up the entire firmware for the picture switcher.
  In essence, it runs such that the microcontroller (Seeeduino XIAO) inderectly
  controls a pair of motors which unravel a long sheet of paper, on which images
  are stored.

  finished 9/3/2022
  by Nicholas Toutges
*/

#include "constants.h"
#include "motor.h"
#include "table.h"
#include <Servo.h>

unsigned long wait_time = 0;      // generic variable that indicates when the device should be waiting
unsigned long next_page_time = 0; // a timeout that, if ever exceeded, will send the device into a STALL condition
unsigned long assist_time = 0;    // a timeout that toggles the non-pulling motor to occasionally enable, and reduce the stress on the pulling-motor
byte current_page = 0;        // stores the page number currently being displayed
byte next_page = 0;           // stores the page number that the device is navigating to
int page_step = 0;            // [-1,1], indicates in which direction the paper roll is traveling
bool is_page_transition = false;  // true if a page-end-marker is currently being detected
bool is_first_page = true;        // true when the machine startes to move the paper roll, false until the paper stops
unsigned char fine_tune_itts = 0; // stores the number of times the machine has "fine tuned" the position of the paper roll
bool are_motors_running = false;      // true if ANY motor is running
bool is_assist_motor_running = false; // true if non-pulling motor is running
bool is_button_pressed = false; // true if button was pressed
byte dark_time = 0;             // 255 if it has been dark long enough to constitute a "dark" condition
Servo buttonServo;

Motor top_motor(REVERSE_1_PIN, ENABLE_1_PIN, false); // pulls paper roll upwards, assists when paper roll traveling downwards
Motor bottom_motor(REVERSE_2_PIN, ENABLE_2_PIN, true); // pulls paper roll downwards, assists when paper roll traveling upwards

enum State {
  UI_S_RESTART,
  UI_S_IDLE,
  UI_S_NEXT_PAGE,
  UI_S_NEW_PAGE,
  UI_S_COARSE_TUNE,
  UI_S_ASSIST_MOTOR,
  UI_S_FINE_TUNE_1,
  UI_S_FINE_TUNE_2,
  UI_S_STALL_1,
  UI_S_STALL_2,
  UI_S_STALL_3
};
State mstate;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PHOTO_PIN, INPUT);

  pinMode(SEED_PIN, INPUT);

  pinMode(REVERSE_1_PIN, OUTPUT);
  pinMode(REVERSE_2_PIN, OUTPUT);
  pinMode(ENABLE_1_PIN, OUTPUT);
  pinMode(ENABLE_2_PIN, OUTPUT);

  pinMode(BUTTON_SERVO_PIN, OUTPUT);

  digitalWrite(REVERSE_1_PIN, LOW);
  digitalWrite(REVERSE_2_PIN, LOW);
  digitalWrite(ENABLE_1_PIN, LOW);
  digitalWrite(ENABLE_2_PIN, LOW);

  buttonServo.attach(BUTTON_SERVO_PIN);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);

  analogReadResolution(12);
  randomSeed( analogRead(SEED_PIN) );

  Serial.begin(115200);

  mstate = UI_S_RESTART;
}

void loop() {
  switch (mstate) {
    // resets timers, stops all motors, and turns off light
    case UI_S_RESTART:
      stopMotors();
      wait_time = millis() + INTERSWITCH_WAIT_TIMEOUT_MS;
      disableLED();
      assist_time = millis(); // start with assist motor on
      mstate = UI_S_IDLE;
      fine_tune_itts = 0;
      setButtonPos(table[current_page][1]);
      Serial.print("Restarted at page ");
      Serial.println(current_page);
      delay(500); // let page tension release to prevent accidental button presses
      break;
    // program waits here until something interesting happens
    case UI_S_IDLE: {
        byte buttonValue = digitalRead(BUTTON_PIN);
        byte switchMode = table[current_page][4];
        if ((wait_time < millis() && bitRead(switchMode, 2)) || (bitRead(switchMode, 1) && isNewNight())) {
          mstate = UI_S_NEXT_PAGE;
          next_page = table[current_page][3];
          if (table[current_page][3] == 255) {
            next_page = random(PAGES);
            mstate = UI_S_NEW_PAGE;
          }
        }
        else if (bitRead(switchMode, 0) && !buttonValue && is_button_pressed) { // button just released
          is_button_pressed = false;
          mstate = UI_S_NEXT_PAGE;
          next_page = table[current_page][2];
          if (table[current_page][2] == 255) {
            next_page = random(PAGES);
            mstate = UI_S_NEW_PAGE;
          }
        }
        else {
          is_button_pressed = buttonValue; // button will stay "pressed" if value EVER spikes
          delay(10); // help to prevent problems with physical button bouncing
        }

        if (switchMode > 7 || switchMode == 0) { // don't know what to do
          mstate = UI_S_STALL_1;
        }
      }
      break;
    // moving to the page number (not index) indicated by the variable [next_page]
    case UI_S_NEXT_PAGE:
      mstate = UI_S_STALL_1; // prevent STALL condition
      Serial.print("Navigating to page index ");
      Serial.print(next_page);
      for (byte i = 0; i < PAGES; i++) {
        if (table[i][0] == next_page) { // next_page holds the index initially
          next_page = i;
          mstate = UI_S_NEW_PAGE;
          Serial.print(" (");
          Serial.print(next_page);
          Serial.println(")");
          break;
        }
      }
      break;
    // initialize page movement
    case UI_S_NEW_PAGE:
      if (next_page == current_page) { // try again
        mstate = UI_S_IDLE;
        break;
      }
      stowButton(); // move arm out of the way
      page_step = (next_page > current_page) ? 1 : -1;
      enableLED();
      startMotors();
      next_page_time = millis() + LONG_NEXT_PAGE_TIMEOUT_MS;
      mstate = UI_S_COARSE_TUNE;
      is_first_page = true;
      is_page_transition = true;
      Serial.print("Running assist on ");
      Serial.print((int) (ASSIST_ON_MS * 100 / (ASSIST_ON_MS + ASSIST_OFF_MS)));
      Serial.println("% Duty Cycle");
      delay(INTERTUNE_WAIT_TIMEOUT_MS); // wait for page to have moved slightly
      break;
    // get paper roll to the correct page, although paper alignment may be imprecise
    case UI_S_COARSE_TUNE:
      if (current_page == next_page) {
        stopMotors();
        wait_time = millis() + INTERTUNE_WAIT_TIMEOUT_MS * 2;
        mstate = UI_S_FINE_TUNE_1;
      }
      else if (isNewPage()) {
        if ((current_page == 0 && page_step < 0) || (current_page == 255 && page_step > 0)) {
          mstate = UI_S_STALL_1; // integer overflow should never happen, thus a stall shall occur
          break;
        }
        current_page += page_step;
        next_page_time = millis() + LONG_NEXT_PAGE_TIMEOUT_MS;
      }
      else if (next_page_time < millis()) {
        mstate = UI_S_STALL_1; // prevent STALL condigiont
      }
      else mstate = UI_S_ASSIST_MOTOR;
      break;
    // occasionally enable the non-pulling motor
    case UI_S_ASSIST_MOTOR:
      mstate = UI_S_COARSE_TUNE;
      if (assist_time > millis()) break;
      if (is_assist_motor_running) {
        stopAssistMotor();
        assist_time = millis() + ASSIST_OFF_MS;
      }
      else {
        startAssistMotor();
        assist_time = millis() + ASSIST_ON_MS;
      }
      break;
    // reverse motor direction to align page precisely with the frame
    case UI_S_FINE_TUNE_1: // assumes page has continued drifting
      if (wait_time > millis()) break;
      page_step *= -1;
      next_page_time = millis() + SHORT_NEXT_PAGE_TIMEOUT_MS;
      isNewPage(); // update is_page_transition
      Serial.print("Tune itteration ");
      Serial.println(fine_tune_itts);
      if (fine_tune_itts >= MAX_TUNE_ITTERATIONS || is_page_transition) {
        mstate = UI_S_RESTART;
        break;
      }
      mstate = UI_S_FINE_TUNE_2;
      break;
    // check if passed over page threshold (bottom of the page / indicator of new pages)
    case UI_S_FINE_TUNE_2:
      if (!are_motors_running) startMotors();
      isNewPage(); // update is_page_transition
      if (is_page_transition) {
        fine_tune_itts++;
        stopMotors();
        wait_time = millis() + INTERTUNE_WAIT_TIMEOUT_MS;
        mstate = UI_S_FINE_TUNE_1;
      }
      else if (next_page_time < millis()) {
        mstate = UI_S_STALL_1;
      }
      break;
    // Something went wrong, and the machine cannot recover from its state, and doing anything may break the physical device
    case UI_S_STALL_1: // requires restart to get out of this state (machine has lost "steps")
      stopMotors();
      Serial.println("Stalled");
      wait_time = millis() + STALL_CYCLE_TIMEOUT_MS;
      mstate = UI_S_STALL_2;
      break;
    // flash LED to indicate problem (off)
    case UI_S_STALL_2:
      if (wait_time > millis()) break;
      disableLED();
      wait_time = millis() + STALL_CYCLE_TIMEOUT_MS;
      mstate = UI_S_STALL_3;
      break;
    // flash LED to indicate problem (on)
    case UI_S_STALL_3:
      if (wait_time > millis()) break;
      enableLED();
      wait_time = millis() + STALL_CYCLE_TIMEOUT_MS;
      mstate = UI_S_STALL_2;

      // debugging
//      mstate = UI_S_RESTART;
      break;
  }
}

// returns true once per new page detected
bool isNewPage() {
  unsigned short lightValue = analogRead(PHOTO_PIN);

#if !NEW_PAGE_LIGHT
  bool newPageStart = lightValue < LOW_LIGHT_THRESHOLD;
  bool newPageEnd = lightValue > HIGH_LIGHT_THRESHOLD;
#else
  bool newPageStart = lightValue > HIGH_LIGHT_THRESHOLD;
  bool newPageEnd = lightValue < LOW_LIGHT_THRESHOLD;
#endif

  if (!is_page_transition && newPageStart) {
    is_page_transition = true;
    return false;
  }
  else if (is_page_transition && newPageEnd) {
    if (is_first_page) {
      is_first_page = false;
      is_page_transition = false;
      return false;
    }
    is_page_transition = false;
    Serial.println("New page!");
    return true;
  }
  return false;
}

// returns true once when code believes it to be "dark"
bool isNewNight() {
  unsigned short lightValue = analogRead(PHOTO_PIN);

  if (lightValue < NIGHT_LIGHT_THRESHOLD) {
    if (dark_time == 254) {
      dark_time = 255;
      return true;
    }
    else {
      dark_time++;
    }
    return false;
  }
  else {
    dark_time = 0;
  }
  return false;
}

// turn on LED
void enableLED() {
  digitalWrite(LED_PIN, HIGH);
}

// turn off LED
void disableLED() {
  digitalWrite(LED_PIN, LOW);
}

// start the top/bottom motor, depending on the direction (page_step)
void startMotors() {
  Serial.println("RUN");
  are_motors_running = true;
  if (page_step > 0) {
    top_motor.start();
  }
  else if (page_step < 0) {
    bottom_motor.start();
  }
}

// stop ALL motors
void stopMotors() {
  Serial.println("RUN STOP");
  are_motors_running = false;
  is_assist_motor_running = false;
  top_motor.stop();
  bottom_motor.stop();
}

// start the bottom/top motor (the non-pulling motor), depending on the direction page_step
void startAssistMotor() {
  is_assist_motor_running = true;
  if (page_step < 0) {
    top_motor.start();
    top_motor.setMotorDirection(false);
  }
  else if (page_step > 0) {
    bottom_motor.start();
    bottom_motor.setMotorDirection(false);
  }
}

// stop ONLY the non-pulling motor
void stopAssistMotor() {
  is_assist_motor_running = false;
  if (page_step > 0) {
    bottom_motor.stop();
  }
  else if (page_step < 0) {
    top_motor.stop();
  }
}

// update the servo to move the button to its new position
void setButtonPos(byte value) {
  byte rot = map(value, 0, 255, MIN_BUTTON_ROT, MAX_BUTTON_ROT);
  buttonServo.write(rot);
}

// move button so the arm is entirely within the bounds of the frame
void stowButton() {
  buttonServo.write(MIN_BUTTON_ROT);
}
