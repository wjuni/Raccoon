#ifndef StepMotor_h
#define StepMotor_h
#include <Arduino.h>
#define AVR_CLOCK 16000 // in kHz
#define STEP_MOTOR_SPEED_MIN 400 // Half Period, in ms
#define STEP_MOTOR_SPEED_MAX 100 // Half Period, in ms
#define STEP_MOTOR_PRESCALER 256  //set prescaler Clk/256

void StepMotor_initialize();
void StepMotor_move(int motor, int speed); // range 0 - 100
void StepMotor_direction(int motor, int dir);
void StepMotor_enable();
void StepMotor_disable();
void StepMotor_start(int motor);
void StepMotor_stop(int motor);

ISR(TIMER1_COMPA_vect);
ISR(TIMER3_COMPA_vect);
ISR(TIMER4_COMPA_vect);
ISR(TIMER5_COMPA_vect);

#endif /* StepMotor_h */
