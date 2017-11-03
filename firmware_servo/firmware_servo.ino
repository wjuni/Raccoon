#include "debug.h"
#include "PktProtocol.h"
#include "SerialComm.h"
#include <Servo.h>
#define READ_PERIOD 1


 
/* GLOBAL */
SerialComm raspicomm(&Serial);
//Compass compass;
Servo linear1, linear2, servo;
long epoch = 0;

/* Prototype */
void packet_handler(PktArduinoV2 *pkt);

void setup() {
    
    raspicomm.begin(115200); delay(10);
    linear1.attach(3);
    linear2.attach(4);
    servo.attach(5);
}

void loop() {
   raspicomm.read(packet_handler);

    delay(READ_PERIOD);
    epoch++;
}

void packet_handler(PktArduinoV2 *pkt) {
  linear1.writeMicroseconds(pkt->linear_servo1 ? pkt->linear_servo1*10 : 1000);
  linear2.writeMicroseconds(pkt->linear_servo2 ? pkt->linear_servo2*10 : 1000);
  servo.write(pkt->servo);
}

