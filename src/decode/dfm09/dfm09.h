#pragma once

#include <stdint.h>

/* Physical parameters */
#define DFM09_BAUDRATE 2500.0

/* Frame parameters */
#define DFM09_SYNCWORD 0x9a995a55
#define DFM09_SYNC_LEN 4
#define DFM09_FRAME_LEN 70

#define DFM09_INTERLEAVING_PTU 7
#define DFM09_INTERLEAVING_GPS 13


typedef struct {
	uint8_t sync[2];
	uint8_t ptu[7];
	uint8_t gps[26];
} __attribute__((packed)) DFM09Frame;


void dfm09_manchester_decode(DFM09Frame *dst, const uint8_t *src);
void dfm09_deinterleave(DFM09Frame *frame);
int dfm09_correct(DFM09Frame *frame);
