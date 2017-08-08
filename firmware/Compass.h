#ifndef Compass_h
#define Compass_h
#include <Wire.h>
#include "HMC5883L.h"

class Compass {
  private:
    HMC5883L sensor;
    
  public:
    float headingDegrees;
    boolean connected;
    Compass();
    void read();
};

#endif
