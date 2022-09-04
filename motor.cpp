#include "Arduino.h"
#include "motor.h"

Motor::Motor() {} // more for defining placeholder motor variables than for actual use as a motor

Motor::Motor(int reversePin, int enablePin) {
  revPin = reversePin;
  startPin = enablePin;
}

Motor::Motor(int reversePin, int enablePin, bool reversed) {
  revPin = reversePin;
  startPin = enablePin;
  isReversed = reversed;
}

bool Motor::isMotorMoving() {
  return isMoving;
}

bool Motor::isMotorReversed() {
  return isReversed;
}

void Motor::start() {
  digitalWrite(startPin, HIGH); // enable motors to spin
  setMotorDirection(true); // set direction for motors to spin in
  isMoving = true;
}

void Motor::stop() {
  // reset all relays
    digitalWrite(revPin, LOW);
    digitalWrite(startPin, LOW);
    isMoving = false;
}

void Motor::setMotorDirection(bool defaultDirection) {
  bool absoluteDirection = defaultDirection ^ isReversed; // invert [defaultDirection] only if motor is wired in reverse
  if (absoluteDirection ) { // go forwards
    digitalWrite(revPin, LOW);
  }
  else { // go backwards
    digitalWrite(revPin, HIGH);
  }
}
