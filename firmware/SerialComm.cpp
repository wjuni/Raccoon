#include <string.h>
#include "SerialComm.h"
#include "debug.h"

SerialComm::SerialComm(HardwareSerial *ser, unsigned long baudrate) : received_packet(), serial_buffer() {
  this->serial = ser;
  this->serial->begin(baudrate);
  memset(this->serial_buffer, 0, BUFFER_SIZE);
  this->buffer_len = 0;
  memset(&received_packet, 0, sizeof(PktArduinoV2));
  memset(serial_buffer, 0, sizeof(serial_buffer));
}

void SerialComm::read(void (*handler)(PktArduinoV2 *)){
  if (this->serial->available() > 0 && BUFFER_SIZE > this->buffer_len) {
    buffer_len += this->serial->readBytes(this->serial_buffer + this->buffer_len, BUFFER_SIZE - this->buffer_len);
    DEBUG_PRINT("Packet Read, Acclen=" + buffer_len);
  }
  if(this->buffer_len >= BUFFER_SIZE) {
    DEBUG_PRINT("New Packet Detected.");
    if(PktArduinoV2_parse_packet(serial_buffer, buffer_len, &(this->received_packet)) > 0){
      // on Packet Received Successful
      DEBUG_PRINT("Packet Read Successful");
      (*handler)(&(this->received_packet));
    }
    this->buffer_len = 0; //clear buffer
  }
}

int SerialComm::write(void *payload, unsigned int len){
  DEBUG_PRINT("Serial Write");
  return this->serial->write((char *)payload, len);
}

