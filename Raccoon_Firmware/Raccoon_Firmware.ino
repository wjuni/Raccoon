#include "PktProtocol.h"
#include "GpsModule.h"
#include "Compass.h"
#include "StepMotor.h"
#include "SerialComm.h"

#define RASPI_REPORT_PERIOD 500
#define READ_PERIOD 10
#define LED_OUT1 32
#define LED_OUT2 33


/* GLOBAL */
SerialComm raspicomm(&Serial1);
GPS gps(&Serial2);
Compass compass;
long epoch = 0;

/* Prototype */
void packet_handler(PktArduino * pkt);

void setup() { 
  Serial.begin(115200);
  StepMotor_initialize();
}

void loop() {
  gps.read();
  compass.read();
  raspicomm.read(packet_handler);
  
  if(epoch >= RASPI_REPORT_PERIOD/READ_PERIOD) {
    epoch = 0;
    PktRaspi p;
    p.gps_lat = gps.data.latitude;
    p.gps_lon = gps.data.latitude;
    p.gps_alt = gps.data.latitude;
    p.gps_spd = gps.data.speed;
    p.gps_fix = gps.data.fix;
    p.voltage = (uint16_t)(((uint32_t)analogRead(A0)*57)/2.048);
    PktRaspi_prepare_packet(&p);  
    raspicomm.write(&p, sizeof(PktRaspi));
  }
  
  delay(10);
  epoch++;
}

void packet_handler(PktArduino * pkt) {
  // do something
  // e.g.  pkt->head_degree
  //       pkt->x_deviation
  if(pkt->mode & (1<<8)) {
    // Boot Complete Broadcast
    digitalWrite(LED_OUT1, HIGH);
  }
  // expected to call StepMotor_move(int motor, int speed); here
}

