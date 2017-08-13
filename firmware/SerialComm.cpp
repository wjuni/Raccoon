#include <string.h>
#include "SerialComm.h"
#include "debug.h"

SerialComm::SerialComm(HardwareSerial *ser) : received_packet(), serial_buffer() {
    this->serial = ser;
    memset(this->serial_buffer, 0, BUFFER_SIZE);
    this->buffer_len = 0;
    memset(&received_packet, 0, sizeof(PktArduinoV2));
    memset(serial_buffer, 0, sizeof(serial_buffer));
}

void SerialComm::begin(unsigned long baudrate) {
    this->serial->begin(baudrate);
}

void SerialComm::read(void (*handler)(PktArduinoV2 *)) {
    if (this->serial->available() > 0 && BUFFER_SIZE > this->buffer_len) {
        // sync
        if(buffer_len == 0) {
            while(this->serial->read() != PKTARDUINO_PREAMBLE);
            this->serial_buffer[buffer_len++] = PKTARDUINO_PREAMBLE;
        }

        buffer_len += this->serial->readBytes(this->serial_buffer + this->buffer_len, BUFFER_SIZE - this->buffer_len);
        DEBUG_PRINT_("Packet Read, Acclen=");
        DEBUG_PRINT(buffer_len);
    }
    if (this->buffer_len >= sizeof(PktArduinoV2)) {
        DEBUG_PRINT_("New Packet Detected Starting With ");
        DEBUG_PRINT_((unsigned int) this->serial_buffer[0]);
        DEBUG_PRINT_(" ");
        DEBUG_PRINT_((unsigned int) this->serial_buffer[1]);
        DEBUG_PRINT_(" ");
        DEBUG_PRINT((unsigned int) this->serial_buffer[2]);

        if (PktArduinoV2_parse_packet(serial_buffer, buffer_len, &(this->received_packet)) > 0) {
            // on Packet Received Successful
            DEBUG_PRINT("Packet Read Successful");
            (*handler)(&(this->received_packet));
        }
        this->buffer_len = 0; //clear buffer
    }
}

int SerialComm::write(void *payload, unsigned int len) {
    DEBUG_PRINT("Serial Write");
    return this->serial->write((char *) payload, len);
}

