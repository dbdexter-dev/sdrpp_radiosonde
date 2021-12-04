#pragma once

#include <inttypes.h>

/* Misaligned bit copy (source is misaligned) */
static void
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


/* Misaligned bit copy (destination is misaligned) */
static void
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
