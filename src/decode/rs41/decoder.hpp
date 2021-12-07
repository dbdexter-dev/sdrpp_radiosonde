#pragma once

#include <dsp/block.h>
#include "decode/common.hpp"
extern "C" {
#include <correct.h>
}
#include "rs41.h"

class RS41Decoder : public dsp::generic_block<RS41Decoder> {
public:
	RS41Decoder();
	RS41Decoder(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx);
	~RS41Decoder();

	void init(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx);
	void setInput(dsp::stream<uint8_t> *in);
	int run() override;

private:
	void *m_ctx;
	void (*m_handler)(SondeData *data, void *ctx);
	dsp::stream<uint8_t> *m_in;
	correct_reed_solomon *m_rs;
	RS41Calibration m_calibData;
	SondeData m_sondeData;
	uint8_t m_calibDataBitmap[sizeof(RS41Calibration)/8/RS41_CALIB_FRAGSIZE+1];
	bool m_calibrated;

	void descramble(RS41Frame *frame);
	bool rsCorrect(RS41Frame *frame);
	bool crcCheck(RS41Subframe *subframe);
	void updateCalibData(RS41Subframe_Status *status);
	void updateSondeData(SondeData *info, RS41Subframe *subframe);

	float temp(RS41Subframe_PTU *ptu);
	float rh(RS41Subframe_PTU *ptu);
	float rh_temp(RS41Subframe_PTU *ptu);
	float pressure(RS41Subframe_PTU *ptu);
};
