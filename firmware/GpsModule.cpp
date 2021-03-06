#include <string.h>
#include "GpsModule.h"
#include "debug.h"

GPS::GPS(HardwareSerial *ser) {
  memset(&this->data, 0, sizeof(GpsData));
  this->data.ser = ser;
}

void GPS::begin() {
    this->data.GPS = new Adafruit_GPS(this->data.ser);
    this->data.GPS->begin(9600);
    this->data.GPS->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    this->data.GPS->sendCommand(PMTK_SET_NMEA_UPDATE_5HZ); // 5 Hz update rate
//  GPS.sendCommand(PGCMD_ANTENNA);
}

void GPS::read() {
  int c = this->data.GPS->read();
#ifdef DEBUG
   UDR0 = c; // write to Serial0
#endif
 if (this->data.GPS->newNMEAreceived()) {
    if (this->data.GPS->parse(this->data.GPS->lastNMEA())) {
      this->data.latitude = this->data.GPS->latitude;
      this->data.longitude = this->data.GPS->longitude;
      this->data.altitude = this->data.GPS->altitude;
      this->data.speed = this->data.GPS->speed;
      this->data.angle = this->data.GPS->angle;
      this->data.fix = this->data.GPS->fix;
    }
  }
}

