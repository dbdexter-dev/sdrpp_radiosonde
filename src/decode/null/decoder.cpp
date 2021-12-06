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
	generic_block<NullDecoder>::registerInput(_in);
	generic_block<NullDecoder>::_block_init = true;
}

void
NullDecoder::setInput(dsp::stream<uint8_t> *in)
{
	generic_block<NullDecoder>::tempStop();
	generic_block<NullDecoder>::unregisterInput(_in);

	_in = in;

	generic_block<NullDecoder>::registerInput(_in);
	generic_block<NullDecoder>::tempStart();
}

int
NullDecoder::run()
{
	assert(generic_block<NullDecoder>::_block_init);
	if (_in->read() < 0) return -1;
	_in->flush();
	return 0;
}
