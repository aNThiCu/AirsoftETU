#include <Arduino.h>
#include <EEPROM.h>

#ifndef MOTOR_DUTY
#define MOTOR_DUTY 100
#endif

#ifndef DEAD_TIME_TICKS
#define DEAD_TIME_TICKS 2
#endif

#define DEADTIME400 __asm__ __volatile__ ("nop \n\t""nop \n\t""nop \n\t""nop \n\t""nop \n\t""nop \n\t""nop \n\t""nop \n\t")

// --- Pins ---
// FOR ATTINY814 ( in_development_ETU_AIO)
#define  TRIGGER   PIN_PB1
#define CYCLE     PIN_PA1
#define MODE      PIN_PB0
//PIN PA4 and PA5 used for output
// --- Data Structures ---

enum modeStates{
  SEMI,
  AUTO,
  BURST,
  BINARY
};

enum state{
  STANDBY,
  MOTORSTART,
  MOTORFULLPOWER,
  MOTORSTOP
};

struct selector {
  enum modeStates pos1 = SEMI;
  enum modeStates pos2 = AUTO;
};

// --- Logic Variables ---
volatile uint8_t burstCounter = 0;
bool hasBraked = true;
selector modes;
enum modeStates selectorState;
enum state motorState = STANDBY;

// --- Time Variables ---
const unsigned long motorJamTimeout = 500;
unsigned long motorJamTime = 0;
const unsigned long motorBrakeTimeout = 1000;
unsigned long motorBrakeTime = 0;

// --- TCD0 Config ---

static uint8_t TCD_PERIOD_TICKS = 156;
static uint8_t TCD_DRIVE_TICKS = ((TCD_PERIOD_TICKS * MOTOR_DUTY) / 100 >  TCD_PERIOD_TICKS - 2 * DEAD_TIME_TICKS - 2) ? TCD_PERIOD_TICKS - 2 * DEAD_TIME_TICKS - 2 : (TCD_PERIOD_TICKS * MOTOR_DUTY) / 100;
static uint8_t TCD_CMPASET_VAL = DEAD_TIME_TICKS-1;
static uint8_t TCD_CMPACLR_VAL = DEAD_TIME_TICKS +  TCD_DRIVE_TICKS-1;
static uint8_t TCD_CMPBSET_VAL = 2 * DEAD_TIME_TICKS + TCD_DRIVE_TICKS-1;
static uint8_t TCD_CMPBCLR_VAL = TCD_PERIOD_TICKS-1;


void TCD0_clear(){
  TCD0.CTRLA = 0;
  while (!(TCD0.STATUS & TCD_ENRDY_bm)) {}
  TCD0.CTRLC      = 0;
  TCD0.CTRLD      = 0;
  TCD0.CTRLE      = 0;
  TCD0.INPUTCTRLA = 0;
  TCD0.INPUTCTRLB = 0;
  TCD0.EVCTRLA    = 0;
  TCD0.EVCTRLB    = 0;
  TCD0.DLYCTRL    = 0;
}

void TCD0_init()
{    
  TCD0_clear();
  TCD0.CTRLB = TCD_WGMODE_ONERAMP_gc;
  TCD0.CMPASET = TCD_CMPASET_VAL;
  TCD0.CMPACLR = TCD_CMPACLR_VAL;
  TCD0.CMPBSET = TCD_CMPBSET_VAL;
  TCD0.CMPBCLR = TCD_CMPBCLR_VAL;
  _PROTECTED_WRITE(TCD0.FAULTCTRL, 0);

  TCD0.CTRLA = TCD_CLKSEL_SYSCLK_gc | TCD_SYNCPRES_DIV1_gc | TCD_CNTPRES_DIV4_gc;
}

void PORT_init()
{
  PORTMUX.CTRLC = 0;
  PORTA.OUTCLR = PIN4_bm | PIN5_bm;
  PORTA.PIN5CTRL |= PORT_INVEN_bm;
  PORTA.DIRSET = PIN4_bm | PIN5_bm;

}

void change_settings(){

}

void setup() {
    // Input Init
  pinMode(TRIGGER, INPUT);
  pinMode(CYCLE, INPUT);
  pinMode(MODE, INPUT);

  if (digitalRead(TRIGGER)==HIGH) change_settings(); //implement settings

  // Output Init
  PORT_init();
  if(MOTOR_DUTY < 90) TCD0_init();


  // Selector init
  // Temp comment for debugging
  // mode_change();
  selectorState = SEMI;

  // Interrupts init
  attachInterrupt(digitalPinToInterrupt(TRIGGER),  trigger_pressed, RISING);
  attachInterrupt(digitalPinToInterrupt(CYCLE), cycle_completed, RISING);
  // Temp comment for debugging
  // attachInterrupt(digitalPinToInterrupt(MODE), mode_change, CHANGE);
}

void  trigger_pressed() {
  if (motorState == STANDBY) {
    burstCounter = 0;
    motorState = MOTORSTART;
  }
}

void cycle_completed() {
  if (motorState == MOTORFULLPOWER) {
    if (selectorState == BURST) burstCounter++;
    motorJamTime = millis();
    motorState = MOTORSTOP;
  }
}

void mode_change() {
  if (digitalRead(MODE) == HIGH) {
    selectorState = modes.pos2;
  } else {
    selectorState = modes.pos1;
  }
}

void motor_on() {
  PORTA.OUTCLR = PIN4_bm | PIN5_bm;
  
  DEADTIME400;

  if(MOTOR_DUTY<90){
    _PROTECTED_WRITE(TCD0.FAULTCTRL, TCD_CMPAEN_bm | TCD_CMPBEN_bm);
    while (!(TCD0.STATUS & TCD_ENRDY_bm)) {}
    TCD0.CTRLA |= TCD_ENABLE_bm;
  } else PORTA.OUTSET = PIN4_bm;

  motorState = MOTORFULLPOWER;
  motorJamTime = millis();
}

void motor_off(){
  PORTA.OUTCLR = (PIN4_bm | PIN5_bm);

  if(MOTOR_DUTY < 90){
    while (!(TCD0.STATUS & TCD_ENRDY_bm)) {}
    TCD0.CTRLA &= ~TCD_ENABLE_bm;
    while (!(TCD0.STATUS & TCD_ENRDY_bm)) {}
    _PROTECTED_WRITE(TCD0.FAULTCTRL, 0);
  }

  DEADTIME400;
  
  PORTA.OUTCLR = PIN4_bm;
  PORTA.OUTSET = PIN5_bm;
  //add delay till break stop
  motorBrakeTime = millis();
  hasBraked = false;
  motorState = STANDBY;
}

void modes_decision(){
  motorState = MOTORFULLPOWER;
  switch (selectorState){
    case SEMI:
      motor_off();
    break;
    case AUTO:
      if(digitalRead( TRIGGER) == HIGH) motor_off();
    break;
    case BURST:
      if(digitalRead( TRIGGER) == HIGH) motor_off();
      else if(burstCounter >= 3) motor_off();
    break;
  }
}

void loop() {
  if (motorState == MOTORSTART) motor_on();
  else if (motorState == MOTORSTOP) modes_decision();

  if(!hasBraked && millis()-motorBrakeTime > motorBrakeTimeout){
   //Clear high side mos after 1 second
   PORTA.OUTCLR = PIN5_bm;
   hasBraked=true;
  }

  // Disabled for debugging purposes
  // if (motorState == MOTORFULLPOWER && (millis() - motorJamTime > motorJamTimeout)) {
  //  // The motor has been running for too long, indicating a jam
  //  motor_off();
  //  while(true){;} //Power cycle required to shoot again
  // }
}