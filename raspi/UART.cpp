//
//  UART.cpp
//  Makerbot-Lanepainter
//
//  Created by 임휘준 on 2017. 7. 23..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#include "UART.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

UART::UART(std::string deviceName) {
    this->deviceName = deviceName;
}

int UART::begin(unsigned long baudrate) {
    uart0_filestream = open(this->deviceName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode
    if (uart0_filestream == -1)
    {
        std::cout << "Error - Unable to open UART : " << deviceName << std::endl;
        return 0;
    }
    
    //CONFIGURE THE UART
    //The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
    //	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
    //	CSIZE:- CS5, CS6, CS7, CS8
    //	CLOCAL - Ignore modem status lines
    //	CREAD - Enable receiver
    //	IGNPAR = Ignore characters with parity errors
    //	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
    //	PARENB - Parity enable
    //	PARODD - Odd parity (else even)
    struct termios options;
    tcgetattr(uart0_filestream, &options);
    long targetbaud = 0;
    if(baudrate == 9600) {
        targetbaud = B9600;
    } else if(baudrate == 115200){
        targetbaud = B115200;
    } else {
        targetbaud = B9600;
    }
    options.c_cflag = targetbaud | CS8 | CLOCAL | CREAD;		//<Set baud rate
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);
    return 1;
}

long UART::write(void *buf, unsigned long len) {
    if (uart0_filestream != -1)
        return ::write(uart0_filestream, buf, len);
    return 0;
}

long UART::read(void *buf, unsigned long len) {
    if (uart0_filestream != -1)
        return ::read(uart0_filestream, buf, len);
    return 0;
}

int UART::read(){
	int b=0;
	
	UART::read(&b,1);
	return b;
}

void UART::close() {
    ::close(uart0_filestream);
}
