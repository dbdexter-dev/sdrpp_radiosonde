#include "utils.h"

void
bitcpy(uint8_t *dst, uint8_t *src, int offset, int bits)
{
	src += offset / 8;
	offset %= 8;

	/* All but last reads */
	for (; bits > 8; bits -= 8) {
		*dst    = *src++ << offset;
		*dst++ |= *src >> (8 - offset);
	}

	/* Last read */
	if (offset + bits < 8) {
		*dst = (*src << offset) & ~((1 << (8 - bits)) - 1);
	} else {
		*dst  = *src++ << offset;
		*dst |= *src >> (8 - offset);
		*dst &= ~((1 << (8-bits)) - 1);
	}
}


void
bitpack(uint8_t *dst, uint8_t *src, int offset, int bits)
{
	dst += offset/8;
	offset %= 8;

	*dst &= ~((1 << (8-offset)) - 1);

	for (; bits > 8; bits -= 8) {
		*dst++ |= *src >> offset;
		*dst    = *src++ << (8 - offset);
	}

	if (offset + bits < 8) {
		*dst = (*src >> offset) & ~((1 << (8-bits)) - 1);
	} else {
		*dst++ |= *src >> offset;
		*dst    = *src << (8 - offset);
		*dst &= ~((1 << (8-bits)) - 1);
	}
}
uint16_t
crc16(uint16_t poly, uint16_t init, uint8_t *data, int len)
{
	int i;
	uint16_t crc = init;

	for(; len > 0; len--) {
		crc ^= *data++ << 8;
		for (i=0; i<8; i++) {
			crc = crc & 0x8000 ? (crc << 1) ^ poly : (crc << 1);
		}
	}

	return crc;
}
