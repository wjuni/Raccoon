#include "Compass.h"
Compass::Compass() {
  if(!this->sensor.begin())
    return;
  
  this->connected = true;
  
  // Set measurement range
  this->sensor.setRange(HMC5883L_RANGE_1_3GA);

  // Set measurement mode
  this->sensor.setMeasurementMode(HMC5883L_CONTINOUS);

  // Set data rate
  this->sensor.setDataRate(HMC5883L_DATARATE_30HZ);

  // Set number of samples averaged
  this->sensor.setSamples(HMC5883L_SAMPLES_8);

  // Set calibration offset. See HMC5883L_calibration.ino
  this->sensor.setOffset(0, 0);
}

void Compass::read() {
  if (!this->connected)
    return;
    
  Vector norm = this->sensor.readNormalize();

  // Calculate heading
  float heading = atan2(norm.YAxis, norm.XAxis);

  // Set declination angle on your location and fix heading
  // You can find your declination on: http://magnetic-declination.com/
  // (+) Positive or (-) for negative
  // For Bytom / Poland declination angle is 4'26E (positive)
  // Formula: (deg + (min / 60.0)) / (180 / M_PI);
  float declinationAngle = - (7.0 + (35.0 / 60.0)) / (180 / M_PI);
  heading += declinationAngle;

  // Correct for heading < 0deg and heading > 360deg
  if (heading < 0)
  {
    heading += 2 * PI;
  }

  if (heading > 2 * PI)
  {
    heading -= 2 * PI;
  }

  // Convert to degrees
  float headingDegrees = heading * 180/M_PI;
  this->headingDegrees = headingDegrees;
}

