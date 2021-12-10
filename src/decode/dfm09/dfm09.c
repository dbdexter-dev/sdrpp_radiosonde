#include <string.h>
#include "dfm09.h"
#include "utils.h"

int parity(uint8_t x);

void
dfm09_manchester_decode(DFM09Frame *dst, const uint8_t *src)
{
	uint8_t *raw_dst = (uint8_t*)dst;
	uint8_t out;
	uint8_t inBits;
	int i;

	out = 0;
	for (i=0; i<8*DFM09_FRAME_LEN; i+=2) {
		bitcpy(&inBits, src, i, 2);
		out = (out << 1) | (inBits & 0x40 ? 1 : 0);

		if (!(i%16)) {
			*raw_dst++ = out;
			out = 0;
		}
	}
}

void
dfm09_deinterleave(DFM09Frame *frame)
{
	DFM09Frame deinterleaved;
	uint8_t *src;
	int i, j, idx;

	memcpy(deinterleaved.sync, frame->sync, sizeof(frame->sync));

	src = frame->ptu;
	for (i=0; i<sizeof(frame->ptu); i++) {
		for (j=0; j<8; j++) {
			idx = (i*8+j) % DFM09_INTERLEAVING_PTU + (i - i%DFM09_INTERLEAVING_PTU);
			deinterleaved.ptu[idx] <<= 1;
			deinterleaved.ptu[idx] |= src[i] >> (7-j) & 0x01;
		}
	}

	src = frame->gps;
	for (i=0; i<sizeof(frame->gps); i++) {
		for (j=0; j<8; j++) {
			idx = (i*8+j) % DFM09_INTERLEAVING_GPS + (i - i%DFM09_INTERLEAVING_GPS);
			deinterleaved.gps[idx] <<= 1;
			deinterleaved.gps[idx] |= src[i] >> (7-j) & 0x01;
		}
	}

	memcpy(frame, &deinterleaved, sizeof(*frame));
}

int
dfm09_correct(DFM09Frame *frame)
{
	const uint8_t hamming_bitmasks[] = {0xaa, 0x66, 0x1e, 0xff};
	uint8_t *src;
	int i, j;
	int errcount, errpos;


	errcount = 0;
	src = frame->ptu;
	for (i=0; i<sizeof(frame->ptu); i++) {
		errpos = 0;
		for (j=0; j<sizeof(hamming_bitmasks); j++) {
			errpos += (1 << j) * parity(src[i] & hamming_bitmasks[j]);
		}

		if (errpos) {
			printf("@%d: ", errpos);
			printf("0x%02x ", src[i]);
			for (j=0; j<sizeof(hamming_bitmasks); j++) {
				printf("%d ", parity(src[i] & hamming_bitmasks[j]));
			}
			printf("\n");

			errcount++;
			src[i] ^= 1 << (8 - errpos);
		}
	}

	src = frame->gps;
	for (i=0; i<sizeof(frame->gps); i++) {
		errpos = 0;
		for (j=0; j<sizeof(hamming_bitmasks); j++) {
			errpos += (1 << j) * parity(src[i] & hamming_bitmasks[j]);
		}

		if (errpos) {
			printf("@%d\n", errpos);
			errcount++;
			src[i] ^= 1 << (8 - errpos);
		}
	}

	return errcount;
}


int
parity(uint8_t x)
{
	int ret;

	for (ret = 0; x; ret++) {
		x &= x-1;
	}

	return ret % 2;
}

