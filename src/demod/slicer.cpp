#include "slicer.hpp"

dsp::Slicer::Slicer(stream<float> *in)
{
	init(in);
}


void
dsp::Slicer::init(stream<float> *in)
{
	_in = in;
	_tmp = 0;
	_offset = 0;

	generic_block<Slicer>::registerInput(_in);
	generic_block<Slicer>::registerOutput(&out);
	generic_block<Slicer>::_block_init = true;
}

void
dsp::Slicer::setInput(stream<float> *in)
{
	assert(generic_block<Slicer>::_block_init);
	generic_block<Slicer>::tempStop();
	generic_block<Slicer>::unregisterInput(_in);

	_in = in;
	_tmp = 0;
	_offset = 0;

	generic_block<Slicer>::registerInput(_in);
	generic_block<Slicer>::tempStart();
}

int
dsp::Slicer::run()
{
	int i, count, outCount;
	count = _in->read();
	if (count < 0) return -1;


	outCount = 0;
	for (i=0; i<count; i++) {
		_tmp = (_tmp << 1) | (_in->readBuf[i] > 0 ? 1 : 0);
		_offset++;

		if (_offset >= 8) {
			out.writeBuf[outCount] = _tmp;
			outCount++;
			_offset = 0;
			_tmp = 0;
		}
	}

	_in->flush();
	if (outCount > 0 && !out.swap(outCount)) return -1;
	return outCount;
}
