#ifndef SerialComm_h
#define SerialComm_h
#include <Arduino.h>
#include "PktProtocol.h"

#define BUFFER_SIZE sizeof(PktArduino)

class SerialComm {
  private:
    HardwareSerial *serial;
    PktArduino received_packet;
    char serial_buffer[BUFFER_SIZE];
    int buffer_len;
  public:
    SerialComm(HardwareSerial *ser);
    void read(void (*handler)(PktArduino *));
    int write(void *payload, int len);
};
#endif /* SerialComm_h */
