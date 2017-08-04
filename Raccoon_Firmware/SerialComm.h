#ifndef SerialComm_h
#define SerialComm_h
#include <Arduino.h>
#include "PktProtocol.h"

#define BUFFER_SIZE sizeof(PktArduinoV2)

class SerialComm {
  private:
    HardwareSerial *serial;
    PktArduinoV2 received_packet;
    char serial_buffer[BUFFER_SIZE];
    int buffer_len;
  public:
    SerialComm(HardwareSerial *ser);
    void read(void (*handler)(PktArduinoV2 *));
    int write(void *payload, int len);
};
#endif /* SerialComm_h */
