#pragma once

#include <inttypes.h>

#define RS41_BAUDRATE 4800
#define RS41_SYNCWORD 0x086d53884469481f
#define RS41_SYNCLEN 8
#define RS41_RS_LEN 48
#define RS41_DATA_LEN 263
#define RS41_XDATA_LEN 198
#define RS41_FRAME_LEN (RS41_SYNCLEN + RS41_RS_LEN + 1 + RS41_DATA_LEN + RS41_XDATA_LEN)

typedef struct {
	uint8_t syncword[RS41_SYNCLEN];
	uint8_t rs_checksum[RS41_RS_LEN];
	uint8_t extended_flag;
	uint8_t data[RS41_DATA_LEN + RS41_XDATA_LEN];
} __attribute__((packed)) RS41Frame;
