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

int PktArduinoV2_parse_packet(const char* buf, unsigned long len, PktArduinoV2 *target) {
    auto *pkt = (PktArduinoV2 *)buf;
    
    //length check
    if(len > sizeof(PktArduino)) {
        DEBUG_PRINT("[FATAL] Packet Length Incorrect");
        return 0;
    }
    
    //header check
    if(pkt->preamble != PKTARDUINO_PREAMBLE) {
        DEBUG_PRINT("[FATAL] Packet PREAMBLE Incorrect");
        return 0;
    }
    
    //crc check
    if(gen_crc16((uint8_t *)buf, sizeof(PktArduino)-sizeof(uint16_t)) != pkt->crc) {
        DEBUG_PRINT("[FATAL] Packet CRC Incorrect");
        return 0;
    }
    memcpy(target, buf, sizeof(PktArduinoV2));
    return 1;
}

void PktArduinoV2_prepare_packet(PktArduinoV2 *target) {
    target->preamble = PKTARDUINO_PREAMBLE;
    target->_reserved = 0;
    target->crc = gen_crc16((uint8_t *)target, sizeof(PktArduinoV2)-sizeof(uint16_t));
    DEBUG_PRINT("[FATAL] Packet Length Incorrect");
}

void PktRaspi_prepare_packet(PktRaspi *target) {
    target->preamble = PKTRASPI_PREAMBLE;
    target->_reserved = 0;
    target->crc = gen_crc16((uint8_t *)target, sizeof(PktRaspi)-sizeof(uint16_t));
}



