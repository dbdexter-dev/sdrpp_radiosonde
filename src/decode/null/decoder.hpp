#pragma once

#include <dsp/block.h>
#include "decode/common.hpp"

class NullDecoder : public dsp::generic_block<NullDecoder> {
public:
	NullDecoder();
	NullDecoder(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx);
	~NullDecoder();

	void init(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx);
	void setInput(dsp::stream<uint8_t> *in);
	int run() override;

private:
	dsp::stream<uint8_t> *m_in;
};
