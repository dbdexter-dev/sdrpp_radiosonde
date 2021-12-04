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
	dsp::Framer _framer;
	correct_reed_solomon *_rs;

	void descramble(RS41Frame *frame);
	bool rsCorrect(RS41Frame *frame);
	bool crcCheck(RS41Subframe *subframe);
	void updateSondeData(SondeData *info, RS41Subframe *subframe);
};
