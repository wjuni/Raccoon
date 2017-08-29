//
//  UART.h
//  Makerbot-Lanepainter
//
//  Created by 임휘준 on 2017. 7. 23..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#ifndef UART_h
#define UART_h
#include <iostream>
class UART {
    std::string deviceName;
    bool isOpen = false;
    int uart0_filestream = -1;
public:
    UART(std::string deviceName);
    int begin(unsigned long baudrate);
    long read(void *buf, unsigned long len);
	int read();
    long write(void *buf, unsigned long len);
    void close();
};


// /dev/ttyAMA0 for Raspi UART

#endif /* UART_h */
