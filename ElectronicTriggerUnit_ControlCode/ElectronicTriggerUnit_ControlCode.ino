//MotorStates
#define STANDBY 0
#define MOTORENABLE 1
#define MOTORFULLPOWER 2
#define MOTORDISABLE 3
//SelectorStates
#define SEMI 0
#define AUTO 1
//Parameters
#define DEBOUNCETIME 20           //increase if debounce isn't properly handled
#define SELECTORLEDTHRESHOLD 55   //led and phototransistor lottery here , hard to figure out without a serial connection 
////pins
// FOR ATTINY816
// int triggerPin = PIN_PB0;
// int cyclePin = PIN_PA1;
// int modePin = PIN_PB1;
// int motorGroundPlane = PIN_PA5;
// FOR ATTINY814
int triggerPin = PIN_PB0;
int cyclePin = PIN_PA1;
int modePin = PIN_PB1;
int motorGroundPlane = PIN_PA5;
//logic
volatile int motorState = STANDBY;
int selectorState = SEMI;
//debounceTiming
volatile unsigned long last_millis = 0;


void setup() {
  //Input Init
  pinMode(triggerPin, INPUT_PULLUP);
  pinMode(cyclePin, INPUT);
  pinMode(modePin, INPUT);
  //Output Init
  pinMode(motorGroundPlane, OUTPUT);
  digitalWrite(motorGroundPlane,LOW);

  attachInterrupt(digitalPinToInterrupt(triggerPin), flag_trigger, FALLING);
  attachInterrupt(digitalPinToInterrupt(cyclePin), flag_cycle, RISING);
}

void flag_trigger() {
  if (motorState == STANDBY && millis() - last_millis > DEBOUNCETIME) {
    motorState = MOTORENABLE;
    last_millis = millis();
  }
}

void flag_cycle() {
  if (motorState == MOTORFULLPOWER)
    motorState = MOTORDISABLE;
}

void motor_on() {
  digitalWrite(motorGroundPlane, HIGH);
  motorState = MOTORFULLPOWER;
}

void motor_off() {
  if (analogRead(modePin) <= SELECTORLEDTHRESHOLD) selectorState = SEMI;  //Comment these 2 lines if
  else selectorState = AUTO;                                              //you don't want to use the selector
  switch (selectorState) {
    case SEMI:
      digitalWrite(motorGroundPlane, LOW);
      motorState = STANDBY;
      break;
    case AUTO:
      if (digitalRead(triggerPin) == 1) {
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
