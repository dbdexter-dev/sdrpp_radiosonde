#include "decoder.hpp"
extern "C" {
#include "utils.h"
#include "dfm09.h"
}

static FILE *file;

DFM09Decoder::DFM09Decoder(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx)
{
	init(in, handler, ctx);
}

DFM09Decoder::~DFM09Decoder()
{
	if (!generic_block<DFM09Decoder>::_block_init) return;

	generic_block<DFM09Decoder>::unregisterInput(m_in);
	generic_block<DFM09Decoder>::_block_init = false;
}

void
DFM09Decoder::init(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx)
{
	file = fopen("/tmp/dfm09.data", "wb");

	m_in = in;
	m_ctx = ctx;
	m_handler = handler;

	generic_block<DFM09Decoder>::registerInput(m_in);
	generic_block<DFM09Decoder>::_block_init = true;
}

void
DFM09Decoder::setInput(dsp::stream<uint8_t>* in)
{
	generic_block<DFM09Decoder>::tempStop();
	generic_block<DFM09Decoder>::unregisterInput(m_in);

	m_in = in;
	m_sondeData.init();

	generic_block<DFM09Decoder>::registerInput(m_in);
	generic_block<DFM09Decoder>::tempStart();
}

int
DFM09Decoder::run()
{
	DFM09Frame frame;
	int count;

	assert(generic_block<DFM09Decoder>::_block_init);

	if ((count = m_in->read()) < 0) return -1;

	dfm09_manchester_decode(&frame, m_in->readBuf);
	dfm09_deinterleave(&frame);
	printf("%d\n", dfm09_correct(&frame));

	fwrite(&frame, 1, sizeof(frame), file);

	m_in->flush();
	return 0;
}
