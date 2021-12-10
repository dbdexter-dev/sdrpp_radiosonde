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
	void parsePTUSubframe(DFM09Subframe_PTU *gps);
	void parseGPSSubframe(DFM09Subframe_GPS *gps);

	void *m_ctx;
	void (*m_handler)(SondeData *data, void *ctx);
	dsp::stream<uint8_t> *m_in;
	struct tm m_gpsTime;
	int m_lastGPS;

	SondeData m_sondeData;
};
