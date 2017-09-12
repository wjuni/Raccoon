//
//  ArduinoCommunicator.hpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 9. 12..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#ifndef ArduinoCommunicator_hpp
#define ArduinoCommunicator_hpp
#include <iostream>
#include "PktProtocol.h"
#include "UART.h"

class ArduinoCommunicator {
private:
    const static int BUFFER_SIZE = sizeof(PktRaspi);
    int buffer_len = 0;
    char serial_buffer[BUFFER_SIZE];
    UART *uart;
public:
    ArduinoCommunicator(std::string deviceName);
    ~ArduinoCommunicator();
    int begin(unsigned long baudrate);
    void recv(void (*handler)(PktRaspi *));
    long send(PktArduinoV2 pkt);
    void close();
};

#endif /* ArduinoCommunicator_hpp */
