#include <Arduino.h>
#include <EEPROM.h>

// --- States and Defines ---
// Motor States
#define STANDBY 0
#define MOTORENABLE 1
#define MOTORFULLPOWER 2
#define MOTORDISABLE 3

// Selector States
#define SEMI 0
#define AUTO 1
#define BURST 2

// EEPROM Address
#define EEPROM_SETTINGS_ADDR 0

// --- Data Structures ---
struct Settings {
  int selector_pos_1 = SEMI;
  int selector_pos_2 = AUTO;
};

// --- Pins ---
// FOR ATTINY814 ( in_development )
int triggerPin = PIN_PB0;
int cyclePin = PIN_PA1;
int modePin = PIN_PB1;
int motorGroundPlane = PIN_PA5;

// --- Logic Variables ---
volatile int motorState = STANDBY;
volatile int burst_counter = 0; // Volatile as it's modified in an ISR
int selectorState;
Settings current_settings;

// --- Failsafe Variables ---
const unsigned long MOTOR_TIMEOUT = 1000; // 1 second
unsigned long motor_start_time = 0;

// --- Placeholder functions for buzzer feedback ---
void signal_enter_settings() { /* TODO: Add buzzer code */ }
void signal_auto_selected() { /* TODO: Add buzzer code */ }
void signal_burst_selected() { /* TODO: Add buzzer code */ }
void signal_saving() { /* TODO: Add buzzer code */ }

void settings_mode() {
  signal_enter_settings();
  
  // Use a temporary struct for selection
  Settings new_settings = current_settings;

  // Signal the current loaded selection
  if (new_settings.selector_pos_2 == AUTO) signal_auto_selected();
  else signal_burst_selected();

  while (true) {
    // Wait for a trigger press
    if (digitalRead(triggerPin) == LOW) {
      unsigned long press_start_time = millis();
      delay(50); // Simple debounce

      // Wait for the trigger to be released
      while (digitalRead(triggerPin) == LOW);
      unsigned long press_duration = millis() - press_start_time;

      if (press_duration > 2000) { // Long press: Save and exit
        EEPROM.put(EEPROM_SETTINGS_ADDR, new_settings);
        signal_saving();
        // Hang until user reconnects battery
        while (true) { delay(1000); }
      } else { // Short press: Cycle selection
        if (new_settings.selector_pos_2 == AUTO) {
          new_settings.selector_pos_2 = BURST;
          signal_burst_selected();
        } else {
          new_settings.selector_pos_2 = AUTO;
          signal_auto_selected();
        }
      }
    }
  }
}

void setup() {
  // Input Init
  pinMode(triggerPin, INPUT);
  pinMode(cyclePin, INPUT);
  pinMode(modePin, INPUT);

  // Check for programming mode entry
  if (digitalRead(triggerPin) == LOW) {
    settings_mode();
  }

  // Load settings from EEPROM
  EEPROM.get(EEPROM_SETTINGS_ADDR, current_settings);

  // Basic validation of loaded EEPROM data
  if (current_settings.selector_pos_1 != SEMI) {
      current_settings.selector_pos_1 = SEMI;
  }
  if (current_settings.selector_pos_2 != AUTO && current_settings.selector_pos_2 != BURST) {
    current_settings.selector_pos_2 = AUTO; // Default to AUTO
  }

  // Output Init
  pinMode(motorGroundPlane, OUTPUT);
  digitalWrite(motorGroundPlane, LOW);

  // Parameters init
  mode_change();

  // Interrupts init
  attachInterrupt(digitalPinToInterrupt(triggerPin), flag_trigger, FALLING);
  attachInterrupt(digitalPinToInterrupt(cyclePin), flag_cycle, RISING);
  attachInterrupt(digitalPinToInterrupt(modePin), mode_change, CHANGE);
}

void flag_trigger() {
  if (motorState == STANDBY) {
    burst_counter = 0; // Reset burst counter on every new trigger pull
    motorState = MOTORENABLE;
  }
}

void flag_cycle() {
  if (motorState == MOTORFULLPOWER) {
    // Increment burst counter here for accuracy, as this marks a completed cycle
    if (selectorState == BURST) {
      burst_counter++;
    }
    motorState = MOTORDISABLE;
  }
}

void mode_change() {
  if (digitalRead(modePin) == HIGH) {
    // Physical selector is in the second position
    selectorState = current_settings.selector_pos_2;
  } else {
    // Physical selector is in the first position
    selectorState = current_settings.selector_pos_1;
  }
}

void motor_on() {
  digitalWrite(motorGroundPlane, HIGH);
  motorState = MOTORFULLPOWER;
  motor_start_time = millis(); // Record cycle start time for failsafe
}

void motor_off() {
  switch (selectorState) {
    case SEMI:
      digitalWrite(motorGroundPlane, LOW);
      motorState = STANDBY;
      break;
    case AUTO:
      if (digitalRead(triggerPin) == HIGH) { // Trigger has been released
        digitalWrite(motorGroundPlane, LOW);
        motorState = STANDBY;
      } else { // Trigger is still pressed, start the next cycle
        motorState = MOTORENABLE;
      }
      break;
    case BURST:
      // Counter is now incremented in flag_cycle ISR
      if (digitalRead(triggerPin) == HIGH) { // Trigger released mid-burst
        digitalWrite(motorGroundPlane, LOW);
        motorState = STANDBY;
      } else if (burst_counter >= 3) { // Burst of 3 is complete
        digitalWrite(motorGroundPlane, LOW);
        motorState = STANDBY;
      } else { // Continue burst
        motorState = MOTORENABLE;
      }
      break;
  }
}

void loop() {
  if (motorState == MOTORENABLE) motor_on();
  else if (motorState == MOTORDISABLE) motor_off();

  // Failsafe: Check for motor timeout
  if (motorState == MOTORFULLPOWER && (millis() - motor_start_time > MOTOR_TIMEOUT)) {
    // The motor has been running for too long, indicating a jam
    digitalWrite(motorGroundPlane, LOW); // Force motor off
    motorState = STANDBY;               // Reset to standby
  }
}