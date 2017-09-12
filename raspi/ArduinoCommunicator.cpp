//
//  ArduinoCommunicator.cpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 9. 12..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#include "ArduinoCommunicator.hpp"

using namespace std;

ArduinoCommunicator::ArduinoCommunicator(std::string deviceName) {
    this->uart = new UART(deviceName);
    memset(serial_buffer, 0, BUFFER_SIZE);
}

ArduinoCommunicator::~ArduinoCommunicator() {
    delete this->uart;
}

int ArduinoCommunicator::begin(unsigned long baudrate) {
    return this->uart->begin(baudrate);
}
void ArduinoCommunicator::recv(void (*handler)(PktRaspi *)) {
    PktRaspi received_packet;
    
    if(BUFFER_SIZE > buffer_len) {
        if(buffer_len == 0) {
            if(uart->read() != PKTRASPI_PREAMBLE)
                return;
            serial_buffer[buffer_len++] = PKTRASPI_PREAMBLE;
        }
        //        buffer_len += this->serial->readBytes(this->serial_buffer + this->buffer_len, BUFFER_SIZE - this->buffer_len);
        long read_byte = uart->read(serial_buffer + buffer_len, BUFFER_SIZE - buffer_len);
        if(read_byte >= 0) {
            cout << "Read Byte = " << read_byte << endl;
            buffer_len += read_byte;
            
            cout << "Packet Read, Acclen=";
            cout << buffer_len << endl;
            
        }
    }
    
    if(buffer_len >= BUFFER_SIZE) {
        cout << "New Packet Detected Starting With ";
        cout << ((unsigned int) serial_buffer[0]);
        cout << " ";
        cout << ((unsigned int) serial_buffer[1]);
        cout << " ";
        cout << ((unsigned int) serial_buffer[2]) << endl;
        
        if (PktRaspi_parse_packet(serial_buffer, buffer_len, &received_packet) > 0) {
            cout << "Packet Read Successful" << endl;
            (*handler)(&received_packet);
        }
        buffer_len = 0; //clear buffer
    }
    
}
long ArduinoCommunicator::send(PktArduinoV2 pkt) {
    PktArduinoV2_prepare_packet(&pkt);
    return uart->write(&pkt, sizeof(PktArduinoV2));
}

void ArduinoCommunicator::close() {
    return uart->close();
}
