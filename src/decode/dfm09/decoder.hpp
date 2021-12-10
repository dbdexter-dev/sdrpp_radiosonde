#pragma once

#include <dsp/block.h>
#include "decode/common.hpp"
extern "C" {
#include "dfm09.h"
}

class DFM09Decoder : public dsp::generic_block<DFM09Decoder> {
public:
	DFM09Decoder() {};
	DFM09Decoder(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx);
	~DFM09Decoder();

	void init(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx);
	void setInput(dsp::stream<uint8_t> *in);
	int run() override;

private:
	void *m_ctx;
	void (*m_handler)(SondeData *data, void *ctx);
	dsp::stream<uint8_t> *m_in;
	SondeData m_sondeData;
};
