#include "StepMotor.h"

static volatile boolean motor_enable[4];
static volatile uint32_t motor_tick[4];

void StepMotor_initialize(){
  // Use 16-bit Timer1,3,4,5 Ch.A
  uint16_t motor_ocr = 0xffff;
  PORTK &= ~0x0f;
  DDRK |= 0x0f;
  
  // enable global motor output
  DDRC |= 1<<6;
  StepMotor_global_enable();
  
  for(int i=0;i<4;i++){
    motor_enable[i] = false;
    motor_tick[i] = 0;
  }
    
  cli();
  
   // stop timer
  TCCR1B &= ~((1<<CS12) | (1<<CS11) | (1<<CS10));
  TCCR3B &= ~((1<<CS32) | (1<<CS31) | (1<<CS30));
  TCCR4B &= ~((1<<CS42) | (1<<CS41) | (1<<CS40));
  TCCR5B &= ~((1<<CS52) | (1<<CS51) | (1<<CS50));
  
  TIMSK1 |= (1 << OCIE1A);
  TIMSK3 |= (1 << OCIE3A);
  TIMSK4 |= (1 << OCIE4A);
  TIMSK5 |= (1 << OCIE5A);
  
  OCR1AH = 0xff; OCR1AL = 0xff;
  OCR3AH = 0xff; OCR3AL = 0xff;
  OCR4AH = 0xff; OCR4AL = 0xff;
  OCR5AH = 0xff; OCR5AL = 0xff;
  
  TCNT1H = 0; TCNT1L = 0;
  TCNT3H = 0; TCNT3L = 0;
  TCNT4H = 0; TCNT4L = 0;
  TCNT5H = 0; TCNT5L = 0;
  
  // restart timer
  TCCR1B |= (1<<CS12) | (0<<CS11) | (0<<CS10); 
  TCCR3B |= (1<<CS32) | (0<<CS31) | (0<<CS30); 
  TCCR4B |= (1<<CS42) | (0<<CS41) | (0<<CS40); 
  TCCR5B |= (1<<CS52) | (0<<CS51) | (0<<CS50); 
  sei();
  
}
static void StepMotor_changeperiod(int motor, int period) {
  uint32_t motor_ocr = (uint32_t)period * AVR_CLOCK / STEP_MOTOR_PRESCALER;
  if(motor_ocr >= (uint32_t) 0xffff){
    StepMotor_stop(motor);
    motor_ocr = 0xffff;
  }
  
  cli();
  if (motor == 1) {
    OCR1AH = (motor_ocr >> 8) & 0xFF;
    OCR1AL = (motor_ocr) & 0xFF;
    TCNT1H = 0; TCNT1L = 0;
  } else if (motor == 2) {
    OCR3AH = (motor_ocr >> 8) & 0xFF;
    OCR3AL = (motor_ocr) & 0xFF;
    TCNT3H = 0; TCNT3L = 0;
  } else if (motor == 3) {
    OCR4AH = (motor_ocr >> 8) & 0xFF;
    OCR4AL = (motor_ocr) & 0xFF;
    TCNT4H = 0; TCNT4L = 0;
  } else if (motor == 4) {
    OCR5AH = (motor_ocr >> 8) & 0xFF;
    OCR5AL = (motor_ocr) & 0xFF;
    TCNT5H = 0; TCNT5L = 0;
  }
  sei();
}
void StepMotor_move(int motor, int speed) {
  if(speed <= 0)
    speed = 0;
  if(speed >= 100)
    speed = 100;
  int scaled_period = STEP_MOTOR_SPEED_MAX + speed * ((STEP_MOTOR_SPEED_MIN - STEP_MOTOR_SPEED_MAX)/100.);
  if(!(PORTC || 1<<6)) PORTC |= 1<<6; // enable driver if driver is off
  motor_enable[motor-1] = (speed != 0); // start or stop motor according to speed
  StepMotor_changeperiod(motor, scaled_period);
}


void StepMotor_direction(int motor, int dir) {
  if(dir)
    PORTK |= (1 << (motor+3));
  else
    PORTK &= ~(1 << (motor+3));
}

void StepMotor_global_enable(){
  PORTC |= 1<<6;
}

void StepMotor_global_disable(){
  PORTC &= ~(1<<6);
}

void StepMotor_start(int motor){
  if(motor >= 0 && motor < 4)
    motor_enable[motor] = true;
}

void StepMotor_stop(int motor){
  if(motor >= 0 && motor < 4)
    motor_enable[motor] = false;
}

ISR(TIMER1_COMPA_vect) {
  TCNT1H = 0; TCNT1L = 0;
  if(motor_enable[0]) {
    PORTK ^= (1<<0);
    motor_tick[0]++;
  }
}
ISR(TIMER3_COMPA_vect) {
  TCNT3H = 0; TCNT3L = 0;
  if(motor_enable[1]) {
    PORTK ^= (1<<1);
    motor_tick[1]++;
  }
}
ISR(TIMER4_COMPA_vect) {
  TCNT4H = 0; TCNT4L = 0;
  if(motor_enable[2]) {
    PORTK ^= (1<<2);
    motor_tick[2]++;
  }
}
ISR(TIMER5_COMPA_vect) {
  TCNT5H = 0; TCNT5L = 0;
  if(motor_enable[3]) {
    PORTK ^= (1<<3);
    motor_tick[3]++;
  }
}
