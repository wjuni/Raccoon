#include "debug.h"
#include "PktProtocol.h"
#include "SerialComm.h"
#include <Servo.h>
#define RASPI_REPORT_PERIOD 500
#define READ_PERIOD 1
#define LED_OUT1 32
#define LED_OUT2 33

#define abs(x) ((x)>0 ? (x) : -(x))

/* GLOBAL */
SerialComm raspicomm(&Serial);
//Compass compass;
Servo linear;
long epoch = 0;

/* Prototype */
void packet_handler(PktArduinoV2 *pkt);

void setup() {
    
    raspicomm.begin(115200); delay(10);
    linear.attach(3);
}

void loop() {
   raspicomm.read(packet_handler);

    delay(READ_PERIOD);
    epoch++;
}

void packet_handler(PktArduinoV2 *pkt) {

    linear.write(pkt->servo);
}

