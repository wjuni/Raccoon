//    L1       R1
//
//    L2       R2
// map L1, R1, L2, R2 as motor 1, 2, 3, 4
#define L1 motor_1_spd
#define R1 motor_2_spd
#define L2 motor_3_spd
#define R2 motor_4_spd
#include "PktProtocol.h"

#define sbi(reg, x) reg |= (0x01 << x)
#define cbi(reg, x) reg &= ~(0x01 << x)
int dir;
int RL;
int vel;
//int stopper;
volatile long time_rise[4];
volatile long time_pulse[4];
PktArduinoV2 pkt;

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(115200);
  cbi(DDRD,3); sbi(PORTD,3);
  cbi(DDRD,4); sbi(PORTD,4);
  cbi(DDRD,5); sbi(PORTD,5);
  cbi(DDRD,6); sbi(PORTD,6);
  sbi(PCICR, PCIE2);
  sbi(PCMSK2, PCINT19);
  sbi(PCMSK2, PCINT20);
  sbi(PCMSK2, PCINT21);
  sbi(PCMSK2, PCINT22);

  pkt.L1 = 0;
  pkt.L2 = 0;
  pkt.R1 = 0;
  pkt.R2 = 0;
  pkt.mode = (1<<8);
  pkt._reserved = 0;  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (time_pulse[3]<1200){
    //enable state

//chack velocity
    if (time_pulse[1] < 1100)
      vel = 0; //stop
    else 
      vel = (time_pulse[1]-1020)/100; //velocity

    if (time_pulse[0]<1200){
      //left
      pkt.L1 = vel;
      pkt.L2 = -vel;
      pkt.R1 = -vel;
      pkt.R2 = vel;
      //L1 : vel
      //L2 : -vel
      //R1 : -vel
      //R2 : vel

    }
    else if (time_pulse[0]>1700){
      //right
      pkt.L1 = -vel;
      pkt.L2 = vel;
      pkt.R1 = vel;
      pkt.R2 = -vel;
      //L1 : -vel
      //L2 : vel
      //R1 : vel
      //R2 : -vel

    }
    else
      //front or retrieve
      if (time_pulse[2]<1200){
        //front 
        pkt.L1 = vel;
        pkt.L2 = vel;
        pkt.R1 = vel;
        pkt.R2 = vel;
        //L1 : vel
        //L2 : vel
        //R1 : vel
        //R2 : vel
      }
      else if (time_pulse[2]>1700){
        //retrieve 
        pkt.L1 = -vel;
        pkt.L2 = -vel;
        pkt.R1 = -vel;
        pkt.R2 = -vel;
        //L1 : -vel
        //L2 : -vel
        //R1 : -vel
        //R2 : -vel
      }
      else{
        //neutral
        pkt.L1 = 0;
        pkt.L2 = 0;
        pkt.R1 = 0;
        pkt.R2 = 0;
        //L1 : 0
        //L2 : 0
        //R1 : 0
        //R2 : 0
      }
  }
  else{
    //all stop
    pkt.L1 = 0;
    pkt.L2 = 0;
    pkt.R1 = 0;
    pkt.R2 = 0;
    //L1 : 0
    //L2 : 0
    //R1 : 0
    //R2 : 0
  }
  PktArduinoV2_prepare_packet(&pkt);
  Serial.write((char *)&pkt, sizeof(pkt));
  delay(50);
}

ISR(PCINT2_vect){
  static char pre_pin;
  char mask;

  mask = PIND^pre_pin;
  pre_pin = PIND;

  if(mask & 0b00001000){
    if (PIND & 0b00001000) time_rise[0]=micros();
    else time_pulse[0] = micros() - time_rise[0];
  }
  if(mask & 0b00010000){
    if (PIND & 0b00010000) time_rise[1]=micros();
    else time_pulse[1] = micros() - time_rise[1];
  }
  if(mask & 0b00100000){
    if (PIND & 0b00100000) time_rise[2]=micros();
    else time_pulse[2] = micros() - time_rise[2];
  }
  if(mask & 0b01000000){
    if (PIND & 0b01000000) time_rise[3]=micros();
    else time_pulse[3] = micros() - time_rise[3];
  }
}

