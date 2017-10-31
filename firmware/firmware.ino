#include "debug.h"
#include "PktProtocol.h"
#include "GpsModule.h"
#include "Compass.h"
#include "StepMotor.h"
#include "SerialComm.h"

#define RASPI_REPORT_PERIOD 500
#define READ_PERIOD 1
#define LED_OUT1 32
#define LED_OUT2 33

#define abs(x) ((x)>0 ? (x) : -(x))

/* GLOBAL */
SerialComm raspicomm(&Serial1);
GPS gps(&Serial2);
//Compass compass;
long epoch = 0;

/* Prototype */
void packet_handler(PktArduinoV2 *pkt);

void setup() {
#if _DEBUG
    Serial.begin(115200);
#endif
    DEBUG_PRINT("Firmware Boot Start...");
    StepMotor_initialize();

    pinMode(LED_OUT1, OUTPUT);
    pinMode(LED_OUT2, OUTPUT);
    DEBUG_PRINT("RaspberryPi Comm Begin...");
    raspicomm.begin(115200); delay(10);
    DEBUG_PRINT("GPS Module Begin...");
    gps.begin(); delay(10);
    
    DEBUG_PRINT("Initialize Complete.");
//    
//  DEBUG_PRINT("Compass Module Begin...");
//    compass.begin(); delay(10);

/*   for(int i=0;i<=100;i+=1){
        StepMotor_move(1,i);
        StepMotor_direction(1, 1);
        StepMotor_move(2, i);
        StepMotor_direction(2, 1);
        StepMotor_move(3, i);
        StepMotor_direction(3, 1);
        StepMotor_move(4, i);
        StepMotor_direction(4, 1);
      delay(70);

    }
     for(int i=100;i>=0;i-=1){
        StepMotor_move(1,i);
        StepMotor_direction(1, 1);
        StepMotor_move(2, i);
        StepMotor_direction(2, 1);
        StepMotor_move(3, i);
        StepMotor_direction(3, 1);
        StepMotor_move(4, i);
        StepMotor_direction(4, 1);
      delay(70);

    }
*/ 
Serial3.begin(115200);
digitalWrite(LED_OUT1, HIGH);
}

void loop() {
   gps.read();
  // compass.read();
   raspicomm.read(packet_handler);

    if (epoch >= RASPI_REPORT_PERIOD / READ_PERIOD) {
//      Serial.println((long)(gps.data.latitude*DEG_MULTIPLIER));
//      Serial.println((long)(gps.data.longitude*DEG_MULTIPLIER));
//      Serial.println((long)(gps.data.altitude*SPD_ALT_MULTIPLIER));
//      Serial.println((long)(gps.data.speed*SPD_ALT_MULTIPLIER));
//      Serial.println((int)gps.data.fix);
        epoch = 0;
        PktRaspi p;
        p.gps_lat = (uint32_t) (gps.data.latitude * DEG_MULTIPLIER);
        p.gps_lon = (uint32_t) (gps.data.longitude * DEG_MULTIPLIER);
        p.gps_alt = (uint16_t) (gps.data.altitude * SPD_ALT_MULTIPLIER);
        p.gps_spd = (uint16_t) (gps.data.speed * SPD_ALT_MULTIPLIER);
        p.gps_fix = (gps.data.fix ? 1 : 0);
        p.voltage = (uint16_t) (((uint32_t) analogRead(A0) * 57) / 2.048);
        PktRaspi_prepare_packet(&p);
        raspicomm.write(&p, sizeof(PktRaspi));
        DEBUG_PRINT("Sending Command.");
 
    }

    delay(READ_PERIOD);
    epoch++;
}

void packet_handler(PktArduinoV2 *pkt) {
//    if (pkt->mode & (1 << 8)) {
//        // Boot Complete Broadcast
//        DEBUG_PRINT("Detected Boot Complete Broadcast.");
//        digitalWrite(LED_OUT1, HIGH);
//    }
//    if (pkt->mode & (1 << 9)) {
//        // Network Complete Broadcast
//        digitalWrite(LED_OUT2, HIGH);
//    }

digitalWrite(LED_OUT2, HIGH);
    DEBUG_PRINT(abs(pkt->motor_2_spd));
    StepMotor_move(1, abs(pkt->motor_1_spd));
    StepMotor_direction(1, pkt->motor_1_spd >= 0);
    StepMotor_move(2, abs(pkt->motor_2_spd));
    StepMotor_direction(2, pkt->motor_2_spd >= 0);
    StepMotor_move(3, abs(pkt->motor_3_spd));
    StepMotor_direction(3, pkt->motor_3_spd >= 0);
    StepMotor_move(4, abs(pkt->motor_4_spd));
    StepMotor_direction(4, pkt->motor_4_spd >= 0);
    Serial3.write((char *)pkt, sizeof(PktArduinoV2));
}

