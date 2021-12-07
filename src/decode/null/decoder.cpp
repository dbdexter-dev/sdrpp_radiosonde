#include "decoder.hpp"

NullDecoder::NullDecoder() {
}

NullDecoder::NullDecoder(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx)
{
	init(in, handler, ctx);
}

NullDecoder::~NullDecoder()
{
	if (!generic_block<NullDecoder>::_block_init) return;
	generic_block<NullDecoder>::stop();
	generic_block<NullDecoder>::_block_init = false;
}

void
NullDecoder::init(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx)
{
	m_in = in;
	generic_block<NullDecoder>::registerInput(m_in);
	generic_block<NullDecoder>::_block_init = true;
}

void
NullDecoder::setInput(dsp::stream<uint8_t> *in)
{
	generic_block<NullDecoder>::tempStop();
	generic_block<NullDecoder>::unregisterInput(m_in);

	m_in = in;

	generic_block<NullDecoder>::registerInput(m_in);
	generic_block<NullDecoder>::tempStart();
}

int
NullDecoder::run()
{
	assert(generic_block<NullDecoder>::_block_init);
	if (m_in->read() < 0) return -1;
	m_in->flush();
	return 0;
}
