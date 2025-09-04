#include <Arduino.h>

//motor states
#define STANDBY 0
#define MOTORENABLE 1
#define MOTORFULLPOWER 2
#define MOTORDISABLE 3
//selector states
#define SEMI 0
#define AUTO 1
////pins

// FOR ATTINY816 ( latest stable)
// int triggerPin = PIN_PB0;
// int cyclePin = PIN_PA1;
// int modePin = PIN_PB1;
// int motorGroundPlane = PIN_PA5;

// FOR ATTINY814 ( in_development )
int triggerPin = PIN_PB0;
int cyclePin = PIN_PA1;
int modePin = PIN_PB1;
int motorGroundPlane = PIN_PA5;
//logic
volatile int motorState = STANDBY;
int selectorState;
//debounceTiming
volatile unsigned long last_millis = 0;


void setup() {
  //Input Init
  pinMode(triggerPin, INPUT);
  pinMode(cyclePin, INPUT);
  pinMode(modePin, INPUT);
  //Output Init
  pinMode(motorGroundPlane, OUTPUT);
  digitalWrite(motorGroundPlane,LOW);
  //Parameters init
  // selectorState=SEMI; //uncomment if you don't want/don't have a fire mode selector and comment the next line
  mode_change();
  //Interrupts init
  attachInterrupt(digitalPinToInterrupt(triggerPin), flag_trigger, FALLING);
  attachInterrupt(digitalPinToInterrupt(cyclePin), flag_cycle, RISING);
  attachInterrupt(digitalPinToInterrupt(modePin), mode_change, CHANGE); // comment if you don't want/don't have a fire mode selector
}

void flag_trigger() {
  if (motorState == STANDBY)
    motorState = MOTORENABLE;
}

void flag_cycle() {
  if (motorState == MOTORFULLPOWER)
    motorState = MOTORDISABLE;
}

void mode_change(){
  if(digitalRead(modePin) == HIGH) selectorState = AUTO;
  else selectorState = SEMI;
}

void motor_on() {
  digitalWrite(motorGroundPlane, HIGH);
  motorState = MOTORFULLPOWER;
}

void motor_off() {
  switch (selectorState) {
    case SEMI:
      digitalWrite(motorGroundPlane, LOW);
      motorState = STANDBY;
      break;
    case AUTO:
      if (digitalRead(triggerPin) == HIGH) {
        digitalWrite(motorGroundPlane, LOW);
        motorState = STANDBY;
      }
      break;
  }
}

void loop() {
  if (motorState == MOTORENABLE) motor_on();
  else if (motorState == MOTORDISABLE) motor_off();
}
