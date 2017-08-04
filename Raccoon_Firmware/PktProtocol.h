//
//  PktProtocol.h
//
//  Created by 임휘준 on 2017. 7. 6..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#ifndef PktProtocol_h
#define PktProtocol_h
#define PKTARDUINO_MULTIPLIER 100000
#define PKTARDUINO_PREAMBLE 0x24
#define PKTRASPI_PREAMBLE 0x31
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

typedef struct {
    uint8_t preamble;
    uint8_t _reserved;
    uint16_t mode;
    int8_t motor_1_spd;
    int8_t motor_2_spd;
    int8_t motor_3_spd;
    int8_t motor_4_spd;
    uint16_t crc;
}  __attribute__((packed)) PktArduinoV2;

// mode bit : Bit 8 = Raspi Boot Complete Broadcast

typedef struct {
    uint8_t preamble;
    uint8_t _reserved;
    uint16_t gps_lat;
    uint16_t gps_lon;
    uint16_t gps_alt;
    uint16_t gps_spd;
    uint8_t gps_fix;
    uint8_t orientation;
    uint16_t voltage;
    uint16_t crc;
}  __attribute__((packed)) PktRaspi;

int PktArduino_parse_packet(const char* buf, unsigned long len, PktArduino *target);
int PktArduinoV2_parse_packet(const char* buf, unsigned long len, PktArduinoV2 *target);
void PktArduino_prepare_packet(PktArduino *target);
void PktArduinoV2_prepare_packet(PktArduinoV2 *target);
void PktRaspi_prepare_packet(PktRaspi *target);
#endif /* PktArduino_h */

