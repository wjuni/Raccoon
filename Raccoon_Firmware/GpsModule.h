#ifndef GpsModule_h
#define GpsModule_h
#include "Adafruit_GPS.h"

typedef struct  {
  float latitude, longitude, altitude, speed, angle;
  boolean fix; 
  Adafruit_GPS *GPS;
  HardwareSerial *ser;
} GpsData;

class GPS {
  public:
    GpsData data;
    
    GPS(HardwareSerial *ser);
    void read();
};
#endif /* GpsModule_h */
