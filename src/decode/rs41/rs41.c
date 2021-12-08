#include <math.h>
#include <string.h>
#include "rs41.h"
#include "utils.h"

/* Pseudorandom sequence, obtained by autocorrelating the extra data found at the end of frames
 * from a radiosonde with ozone sensor */
static const uint8_t _prn[RS41_PRN_PERIOD] = {
	0x96, 0x83, 0x3e, 0x51, 0xb1, 0x49, 0x08, 0x98,
	0x32, 0x05, 0x59, 0x0e, 0xf9, 0x44, 0xc6, 0x26,
	0x21, 0x60, 0xc2, 0xea, 0x79, 0x5d, 0x6d, 0xa1,
	0x54, 0x69, 0x47, 0x0c, 0xdc, 0xe8, 0x5c, 0xf1,
	0xf7, 0x76, 0x82, 0x7f, 0x07, 0x99, 0xa2, 0x2c,
	0x93, 0x7c, 0x30, 0x63, 0xf5, 0x10, 0x2e, 0x61,
	0xd0, 0xbc, 0xb4, 0xb6, 0x06, 0xaa, 0xf4, 0x23,
	0x78, 0x6e, 0x3b, 0xae, 0xbf, 0x7b, 0x4c, 0xc1
};


void
rs41_descramble(RS41Frame *frame)
{
	int i, j;
	uint8_t tmp;
	uint8_t *rawFrame = (uint8_t*) frame;

	/* Reorder bits in the frame and XOR with PRN sequence */
	for (i=0; i<sizeof(*frame); i++) {
		tmp = 0;
		for (j=0; j<8; j++) {
			tmp |= ((rawFrame[i] >> (7-j)) & 0x1) << j;
		}
		rawFrame[i] = 0xFF ^ tmp ^ _prn[i % RS41_PRN_PERIOD];
	}
}

bool
rs41_correct(RS41Frame *frame, correct_reed_solomon *rs)
{
	bool valid;
	int i, block, chunk_len;
	uint8_t rsBlock[RS41_REEDSOLOMON_N];

	if (frame->extended_flag != RS41_FLAG_EXTENDED) {
		chunk_len = (RS41_DATA_LEN + 1) / RS41_REEDSOLOMON_INTERLEAVING;
		memset(rsBlock, 0, RS41_REEDSOLOMON_N);
	} else {
		chunk_len = RS41_REEDSOLOMON_K;
	}

	valid = true;
	for (block=0; block<RS41_REEDSOLOMON_INTERLEAVING; block++) {
		/* Deinterleave */
		for (i=0; i<chunk_len; i++) {
			rsBlock[i] = frame->data[RS41_REEDSOLOMON_INTERLEAVING*i + block - 1];
		}
		for (i=0; i<RS41_REEDSOLOMON_T; i++) {
			rsBlock[RS41_REEDSOLOMON_K+i] = frame->rs_checksum[i + RS41_REEDSOLOMON_T*block];
		}

		/* Error correct */
		if (correct_reed_solomon_decode(rs, rsBlock, RS41_REEDSOLOMON_N, rsBlock) < 0) valid = false;

		/* Reinterleave */
		for (i=0; i<chunk_len; i++) {
			frame->data[RS41_REEDSOLOMON_INTERLEAVING*i + block - 1] = rsBlock[i];
		}
		for (i=0; i<RS41_REEDSOLOMON_T; i++) {
			frame->rs_checksum[i + RS41_REEDSOLOMON_T*block] = rsBlock[RS41_REEDSOLOMON_K+i];
		}
	}

	return valid;
}


bool
rs41_crc_check(RS41Subframe *subframe)
{
	uint16_t checksum = crc16(CCITT_FALSE_POLY, CCITT_FALSE_INIT, subframe->data, subframe->len);
	uint16_t expected = subframe->data[subframe->len] | subframe->data[subframe->len+1] << 8;

	return checksum == expected;
}

float
rs41_temp(RS41Subframe_PTU *ptu, RS41Calibration *calib)
{
	const float adc_main = (uint32_t)ptu->temp_main[0]
	                      | (uint32_t)ptu->temp_main[1] << 8
	                      | (uint32_t)ptu->temp_main[2] << 16;
	const float adc_ref1 = (uint32_t)ptu->temp_ref1[0]
	                      | (uint32_t)ptu->temp_ref1[1] << 8
	                      | (uint32_t)ptu->temp_ref1[2] << 16;
	const float adc_ref2 = (uint32_t)ptu->temp_ref2[0]
	                      | (uint32_t)ptu->temp_ref2[1] << 8
	                      | (uint32_t)ptu->temp_ref2[2] << 16;

	float adc_raw, r_raw, r_t, t_uncal, t_cal;
	int i;

	/* If no reference or no calibration data, retern */
	if (adc_ref2 - adc_ref1 == 0) return NAN;

	/* Compute ADC gain and bias */
	adc_raw = (adc_main - adc_ref1) / (adc_ref2 - adc_ref1);

	/* Compute resistance */
	r_raw = calib->t_ref[0] + (calib->t_ref[1] - calib->t_ref[0])*adc_raw;
	r_t = r_raw * calib->t_calib_coeff[0];

	/* Compute temperature based on corrected resistance */
	t_uncal = calib->t_temp_poly[0]
	     + calib->t_temp_poly[1]*r_t
	     + calib->t_temp_poly[2]*r_t*r_t;

	t_cal = 0;
	for (i=6; i>0; i--) {
		t_cal *= t_uncal;
		t_cal += calib->t_calib_coeff[i];
	}
	t_cal += t_uncal;

	return t_cal;
}

float
rs41_rh(RS41Subframe_PTU *ptu, RS41Calibration *calib)
{
	float adc_main = (uint32_t)ptu->humidity_main[0]
	                       | (uint32_t)ptu->humidity_main[1] << 8
	                       | (uint32_t)ptu->humidity_main[2] << 16;
	float adc_ref1 = (uint32_t)ptu->humidity_ref1[0]
	                       | (uint32_t)ptu->humidity_ref1[1] << 8
	                       | (uint32_t)ptu->humidity_ref1[2] << 16;
	float adc_ref2 = (uint32_t)ptu->humidity_ref2[0]
	                       | (uint32_t)ptu->humidity_ref2[1] << 8
	                       | (uint32_t)ptu->humidity_ref2[2] << 16;

	int i, j;
	float f1, f2;
	float adc_raw, c_raw, c_cal, rh_uncal, rh_cal, rh_temp_uncal, rh_temp_cal, t_temp;

	if (adc_ref2 - adc_ref1 == 0) return NAN;

	/* Get RH sensor temperature and actual temperature */
	rh_temp_uncal = rs41_rh_temp(ptu, calib);
	t_temp = rs41_temp(ptu, calib);

	/* Compute RH calibrated temperature */
	rh_temp_cal = 0;
	for (i=6; i>0; i--) {
		rh_temp_cal *= rh_temp_uncal;
		rh_temp_cal += calib->th_calib_coeff[i];
	}
	rh_temp_cal += rh_temp_uncal;

	/* Get raw capacitance of the RH sensor */
	adc_raw = (adc_main - adc_ref1) / (adc_ref2 - adc_ref1);
	c_raw = calib->rh_ref[0] + adc_raw * (calib->rh_ref[1] - calib->rh_ref[0]);
	c_cal = (c_raw / calib->rh_cap_calib[0] - 1) * calib->rh_cap_calib[1];

	/* Derive raw RH% from capacitance and temperature response */
	rh_uncal = 0;
	rh_temp_cal = (rh_temp_cal - 20) / 180;
	f1 = 1;
	for (i=0; i<7; i++) {
		f2 = 1;
		for (j=0; j<6; j++) {
			rh_uncal += f1 * f2 * calib->rh_calib_coeff[i][j];
			f2 *= rh_temp_cal;
		}
		f1 *= c_cal;
	}

	/* Account for different temperature between air and RH sensor */
	rh_cal = rh_uncal * wv_sat_pressure(rh_temp_uncal) / wv_sat_pressure(t_temp);
	return fmax(0.0, fmin(100.0, rh_cal));
}

float
rs41_rh_temp(RS41Subframe_PTU *ptu, RS41Calibration *calib)
{
	const float adc_main = (uint32_t)ptu->temp_humidity_main[0]
	                      | (uint32_t)ptu->temp_humidity_main[1] << 8
	                      | (uint32_t)ptu->temp_humidity_main[2] << 16;
	const float adc_ref1 = (uint32_t)ptu->temp_humidity_ref1[0]
	                      | (uint32_t)ptu->temp_humidity_ref1[1] << 8
	                      | (uint32_t)ptu->temp_humidity_ref1[2] << 16;
	const float adc_ref2 = (uint32_t)ptu->temp_humidity_ref2[0]
	                      | (uint32_t)ptu->temp_humidity_ref2[1] << 8
	                      | (uint32_t)ptu->temp_humidity_ref2[2] << 16;

	float adc_raw, r_raw, r_t, t_uncal;

	/* If no reference or no calibration data, retern */
	if (adc_ref2 - adc_ref1 == 0) return NAN;
	if (!calib->t_ref[0] || !calib->t_ref[1]) return NAN;

	/* Compute ADC gain and bias */
	adc_raw = (adc_main - adc_ref1) / (adc_ref2 - adc_ref1);

	/* Compute resistance */
	r_raw = calib->t_ref[0] + adc_raw * (calib->t_ref[1] - calib->t_ref[0]);
	r_t = r_raw * calib->th_calib_coeff[0];

	/* Compute temperature based on corrected resistance */
	t_uncal = calib->th_temp_poly[0]
	     + calib->th_temp_poly[1]*r_t
	     + calib->th_temp_poly[2]*r_t*r_t;

	return t_uncal;
}

float
rs41_pressure(RS41Subframe_PTU *ptu, RS41Calibration *calib)
{
	/* TODO */
	return 0;
}
