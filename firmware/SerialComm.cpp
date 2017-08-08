#include <string.h>
#include "SerialComm.h"


SerialComm::SerialComm(HardwareSerial *ser) {
  this->serial = ser;
  memset(this->serial_buffer, 0, BUFFER_SIZE);
  this->buffer_len = 0;
}

void SerialComm::read(void (*handler)(PktArduinoV2 *)){
  if (this->serial->available() && BUFFER_SIZE > this->buffer_len) {
    buffer_len += this->serial->readBytes(this->serial_buffer + this->buffer_len, BUFFER_SIZE - this->buffer_len);
  }
  if(this->buffer_len >= BUFFER_SIZE) {
    if(PktArduinoV2_parse_packet(serial_buffer, buffer_len, &(this->received_packet))){
      // on Packet Received Successful
      (*handler)(&(this->received_packet));
    }
    this->buffer_len = 0; //clear buffer
  }
}

int SerialComm::write(void *payload, int len){
  return this->serial->write((char *)payload, len);
}

