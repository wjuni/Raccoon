//
//  PktArduino.h
//
//  Created by 임휘준 on 2017. 7. 6..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#ifndef PktArduino_h
#define PktArduino_h
#define PKTARDUINO_MULTIPLIER 100000
#define PKTARDUINO_PREAMBLE 0x24
#include <stdint.h>
typedef struct {
    uint8_t preamble;
    uint8_t _reserved;
    uint16_t mode;
    int32_t head_degree;
    int32_t x_deviation;
    int32_t y_deviation;
    uint16_t crc;
}  __attribute__((packed)) PktArduino;

int parse_packet(const char* buf, unsigned long len, PktArduino *target);
void prepare_packet(PktArduino *target);

#endif /* PktArduino_h */
