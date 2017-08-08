#define sbi(reg, x) reg |= (0x01 << x)
#define cbi(reg, x) reg &= ~(0x01 << x)
int dir;
int RL;
int vel;
//int stopper;
volatile long time_rise[4];
volatile long time_pulse[4];

void setup() {
  // put your setup code here, to run once:
  
  sbi(DDRB,1); //enable pin low-allow pin8
  sbi(DDRB,2); //1 pin9
  sbi(DDRB,3);//2 pin10
  sbi(DDRB,4); //3 pin11
  sbi(DDRB,5); //4 pin12
  Serial.begin(9600);
  cbi(DDRD,3); sbi(PORTD,3);
  cbi(DDRD,4); sbi(PORTD,4);
  cbi(DDRD,5); sbi(PORTD,5);
  cbi(DDRD,6); sbi(PORTD,6);
  sbi(PCICR, PCIE2);
  sbi(PCMSK2, PCINT19);
  sbi(PCMSK2, PCINT20);
  sbi(PCMSK2, PCINT21);
  sbi(PCMSK2, PCINT22);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (time_pulse[2]<1200)
    dir = 0; //front
  else if (time_pulse[2]<1700)
    dir = 1; //stop
  else
    dir = 2; //back

  if (time_pulse[3]<1200)
    cbi(PORTB,1);
  else
    sbi(PORTB,1);
    
  if (time_pulse[0]<1200)
    RL = 0; //left
  else if (time_pulse[0]>1700)
    RL = 2; //right
  else
    RL = 1; //middle
  
  if (time_pulse[1] < 1100)
    vel = 0; //front
  else 
    vel = (time_pulse[1]-1020)/100; //stop
//  Serial.write(dir);
//  Serial.write(RL);
  if (dir =1){
    cbi(PORTB,1);
  }
  else{
    if (RL = 0){
        sbi(PORTB,2);
        cbi(PORTB,3);
        sbi(PORTB,4);
        cbi(PORTB,5);
    }
    else if (RL = 2){
        cbi(PORTB,2);
        sbi(PORTB,3);
        cbi(PORTB,4);
        sbi(PORTB,5);
    }
    else{
        if (dir=0){
          sbi(PORTB,2);
          sbi(PORTB,3);
          sbi(PORTB,4);
          sbi(PORTB,5);
        }
        else if (dir=2){
          cbi(PORTB,2);
          cbi(PORTB,3);
          cbi(PORTB,4);
          cbi(PORTB,5);
        }
    }
  }
  Serial.write(vel);
//  Serial.print(dir);
//  Serial.print(RL);
//  Serial.print(vel);
//  Serial.print(stopper);
//  Serial.print('\n');
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

