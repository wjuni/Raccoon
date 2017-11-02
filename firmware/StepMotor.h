#ifndef StepMotor_h
#define StepMotor_h
#include <Arduino.h>
#define AVR_CLOCK 16 // in MHz
#define STEP_MOTOR_SPEED_MIN 1000 // Half Period, in us - deprecated
#define STEP_MOTOR_SPEED_MAX 13  // Half Period, in us - deprecated

#define STEP_MOTOR_FREQ_MAX 10 // in kHz
#define STEP_MOTOR_FREQ_MIN 3 // in kHz


#define STEP_MOTOR_PRESCALER 64  //set prescaler Clk/64

void StepMotor_initialize();
void StepMotor_move(int motor, int speed); // range 0 - 100
void StepMotor_direction(int motor, int dir);
void StepMotor_global_enable();
void StepMotor_global_disable();
void StepMotor_start(int motor);
void StepMotor_stop(int motor);

ISR(TIMER1_COMPA_vect);
ISR(TIMER3_COMPA_vect);
ISR(TIMER4_COMPA_vect);
ISR(TIMER5_COMPA_vect);

#endif /* StepMotor_h */
