#pragma once

#include <dsp/block.h>
#include "decode/common.hpp"
#include "decode/framer.hpp"
extern "C" {
#include <correct.h>
}
#include "rs41.h"

class RS41Decoder : public dsp::generic_block<RS41Decoder> {
public:
	RS41Decoder();
	RS41Decoder(dsp::stream<uint8_t> *in);
	~RS41Decoder();

	void init(dsp::stream<uint8_t> *in);
	void setInput(dsp::stream<uint8_t> *in);
	int run() override;

	dsp::stream<SondeData> out;

private:
	dsp::stream<uint8_t> *_in;
	correct_reed_solomon *_rs;
	RS41Calibration _calibData;
	uint8_t _calibDataBitmap[sizeof(RS41Calibration)/8/RS41_CALIB_FRAGSIZE+1];
	bool _calibrated;

	void descramble(RS41Frame *frame);
	bool rsCorrect(RS41Frame *frame);
	bool crcCheck(RS41Subframe *subframe);
	void updateCalibData(RS41Subframe_Status *status);
	void updateSondeData(SondeData *info, RS41Subframe *subframe);
};
