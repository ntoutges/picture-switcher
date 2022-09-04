/*
  This class tracks data bout, and controls a basic DC motor.
  It can start, stop, and reverse a motor with two pins:
    - rev pin (connected to reversing circuitry)
    - start pin (connects motor to power)
*/

#ifndef MOTOR_CLASS_H
#define MOTOR_CLASS_H

class Motor {
  public:
  Motor();
  Motor(int reversePin, int enablePin);
  Motor(int reversePin, int enablePin, bool reversed);

  bool isMotorMoving();
  bool isMotorReversed();
  void start();
  void stop();
  void setMotorDirection(bool defaultDirection);

  private:
  int revPin = 0;
  int startPin = 0;
  bool isReversed = false;
  bool isMoving;
};

#endif
