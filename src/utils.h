#pragma once

#include <stdint.h>
#define CCITT_FALSE_INIT 0xFFFF
#define CCITT_FALSE_POLY 0x1021

/* CRC16 checksum */
uint16_t crc16(uint16_t poly, uint16_t init, uint8_t *data, int len);

/* Misaligned bit copy (source is misaligned) */
void bitcpy(uint8_t *dst, uint8_t *src, int offset, int bits);

/* Misaligned bit copy (destination is misaligned) */
void bitpack(uint8_t *dst, uint8_t *src, int offset, int bits);

