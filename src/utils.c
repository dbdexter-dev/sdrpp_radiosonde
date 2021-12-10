#include <math.h>
#include "utils.h"

void
bitcpy(uint8_t *dst, const uint8_t *src, int offset, int bits)
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
bitpack(uint8_t *dst, const uint8_t *src, int offset, int bits)
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

uint64_t
bitmerge(uint8_t *data, int nbits)
{
	uint64_t ret = 0;

	for (; nbits >= 8; nbits-=8) {
		ret = (ret << 8) | *data++;
	}

	return (ret << nbits) | (*data >> (7 - nbits));
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

float
altitude_to_pressure(float alt)
{
	return 1013.25f * powf(1.0f - 2.25577f * 1e-5f * alt, 5.25588f);
}

float
pressure_to_altitude(float pressure)
{
	return 44330 * (1 - powf((pressure / 1013.25f), 1/5.25588f));
}

float
dewpt(float temp, float rh)
{
	const float tmp = (logf(rh / 100.0f) + (17.27f * temp / (237.3f + temp))) / 17.27f;
	return 237.3f * tmp  / (1 - tmp);
}

float
sat_mixing_ratio(float temp, float p)
{
	const float wv_pressure = 610.97e-3 * expf((17.625*temp)/(temp+243.04));
	const float wice_pressure = 611.21e-3 * expf((22.587*temp)/(temp+273.86));

	if (temp < 0) {
		return 621.97 * wice_pressure / (p - wice_pressure);
	} else {
		return 621.97 * wv_pressure / (p - wv_pressure);
	}
}

float
wv_sat_pressure(float temp)
{
	const float coeffs[] = {-0.493158, 1 + 4.6094296e-3, -1.3746454e-5, 1.2743214e-8};
	float T, p;
	int i;

	temp += 273.15f;

	T = 0;
	for (i=(sizeof(coeffs)/sizeof(*coeffs))-1; i>=0; i--) {
		T *= temp;
		T += coeffs[i];
	}

	p = expf(-5800.2206f / T
		  + 1.3914993f
		  + 6.5459673f * logf(T)
		  - 4.8640239e-2f * T
		  + 4.1764768e-5f * T * T
		  - 1.4452093e-8f * T * T * T);

	return p / 100.0f;

}
