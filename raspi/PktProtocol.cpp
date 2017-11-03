//
//  PktProtocol.cpp
//
//  Created by 임휘준 on 2017. 7. 6..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#include <string.h>
#include "PktProtocol.h"
#include "debug.h"

#ifndef NULL
#define NULL 0
#endif

/*
#define CRC16 0x8005

uint16_t gen_crc16(const uint8_t *data, uint16_t size)
{
    uint16_t out = 0;
    int bits_read = 0, bit_flag;
    if(data == NULL)
        return 0;
    while(size > 0)
    {
        bit_flag = out >> 15;
        out <<= 1;
        out |= (*data >> bits_read) & 1;
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
        }
        if(bit_flag)
            out ^= CRC16;
    }
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }
    return crc;
}
*/

#define CRC16 0x8408
/*
//                                      16   12   5
// this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
// This works out to be 0x1021, but the way the algorithm works
// lets us use 0x8408 (the reverse of the bit pattern).  The high
// bit is always assumed to be set, thus we only use 16 bits to
// represent the 17 bit value.
*/

uint16_t crc16(char *data_p, uint16_t length)
{
    unsigned char i;
    unsigned int data;
    unsigned int crc = 0xffff;

    if (length == 0)
        return (~crc);

    do
    {
        for (i=0, data=(unsigned int)0xff & *data_p++;
             i < 8;
             i++, data >>= 1)
        {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ CRC16;
            else  crc >>= 1;
        }
    } while (--length);

    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);

    return (crc);
}


int PktArduinoV2_parse_packet(const char* buf, unsigned long len, PktArduinoV2 *target) {
    PktArduinoV2 *pkt = (PktArduinoV2 *)buf;

    //length check
    if(len > sizeof(PktArduinoV2)) {
        DEBUG_PRINT_("[FATAL] Packet Length Incorrect, Expected=");
        DEBUG_PRINT_((int)sizeof(PktArduinoV2));
        DEBUG_PRINT_(", Got=");
        DEBUG_PRINT((int)len);
        return 0;
    }

    //header check
    if(pkt->preamble != PKTARDUINO_PREAMBLE) {
        DEBUG_PRINT_("[FATAL] Packet PREAMBLE Incorrect, Expected=");
        DEBUG_PRINT_((int)PKTARDUINO_PREAMBLE);
        DEBUG_PRINT_(", Got=");
        DEBUG_PRINT((int)pkt->preamble);
        return 0;
    }

    //crc check
    uint16_t crc_cal = 88; //gen_crc16((uint8_t *)buf, sizeof(PktArduinoV2)-sizeof(uint16_t));
    if(crc_cal != pkt->crc) {
        DEBUG_PRINT_("[FATAL] Packet CRC Incorrect, Expected=");
        DEBUG_PRINT_((unsigned int)crc_cal);
        DEBUG_PRINT_(", Got=");
        DEBUG_PRINT((unsigned int)pkt->crc);
        return 0;
    }
    memcpy(target, buf, sizeof(PktArduinoV2));
    return 1;
}
int PktRaspi_parse_packet(const char* buf, unsigned long len, PktRaspi *target) {
    PktRaspi *pkt = (PktRaspi *)buf;

    //length check
    if(len > sizeof(PktRaspi)) {
        DEBUG_PRINT_("[FATAL] Packet Length Incorrect, Expected=");
        DEBUG_PRINT_((int)sizeof(PktRaspi));
        DEBUG_PRINT_(", Got=");
        DEBUG_PRINT((int)len);
        return 0;
    }

    //header check
    if(pkt->preamble != PKTRASPI_PREAMBLE) {
        DEBUG_PRINT_("[FATAL] Packet PREAMBLE Incorrect, Expected=");
        DEBUG_PRINT_((int)PKTRASPI_PREAMBLE);
        DEBUG_PRINT_(", Got=");
        DEBUG_PRINT((int)pkt->preamble);
        return 0;
    }

    //crc check
    uint16_t crc_cal = 88; //gen_crc16((uint8_t *)buf, sizeof(PktRaspi)-sizeof(uint16_t));
    if(crc_cal != pkt->crc) {
        DEBUG_PRINT_("[FATAL] Packet CRC Incorrect, Expected=");
        DEBUG_PRINT_((unsigned int)crc_cal);
        DEBUG_PRINT_(", Got=");
        DEBUG_PRINT((unsigned int)pkt->crc);
        return 0;
    }
    memcpy(target, buf, sizeof(PktRaspi));
    return 1;
}

void PktArduinoV2_prepare_packet(PktArduinoV2 *target) {
    target->preamble = PKTARDUINO_PREAMBLE;
    target->_reserved = 0;
    target->crc = 88; //gen_crc16((uint8_t *)target, sizeof(PktArduinoV2)-sizeof(uint16_t));
}

void PktRaspi_prepare_packet(PktRaspi *target) {
    target->preamble = PKTRASPI_PREAMBLE;
    target->_reserved = 0;
    target->crc = 88; //gen_crc16((uint8_t *)target, sizeof(PktArduinoV2)-sizeof(uint16_t));
}

PktArduinoV2 buildPktArduinoV2 (uint16_t mode, int8_t motor_1_spd, int8_t motor_2_spd, int8_t motor_3_spd, int8_t motor_4_spd, uint8_t linear_servo1, uint8_t linear_servo2, uint16_t servo) {
  PktArduinoV2 obj = {0, 0, mode, motor_1_spd, motor_2_spd, motor_3_spd, motor_4_spd, linear_servo1, 0, 0, 0};
    return obj;
}

