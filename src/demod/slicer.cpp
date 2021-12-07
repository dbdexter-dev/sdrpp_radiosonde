#include "slicer.hpp"

dsp::Slicer::Slicer(stream<float> *in)
{
	init(in);
}

dsp::Slicer::~Slicer()
{
	if (!generic_block<Slicer>::_block_init) return;
	generic_block<Slicer>::stop();
	generic_block<Slicer>::unregisterInput(m_in);
	generic_block<Slicer>::unregisterOutput(&out);
}


void
dsp::Slicer::init(stream<float> *in)
{
	m_in = in;
	m_tmp = 0;
	m_offset = 0;

	generic_block<Slicer>::registerInput(m_in);
	generic_block<Slicer>::registerOutput(&out);
	generic_block<Slicer>::_block_init = true;
}

void
dsp::Slicer::setInput(stream<float> *in)
{
	assert(generic_block<Slicer>::_block_init);
	generic_block<Slicer>::tempStop();
	generic_block<Slicer>::unregisterInput(m_in);

	m_in = in;
	m_tmp = 0;
	m_offset = 0;

	generic_block<Slicer>::registerInput(m_in);
	generic_block<Slicer>::tempStart();
}

int
dsp::Slicer::run()
{
	int i, count, outCount;
	count = m_in->read();
	if (count < 0) return -1;


	outCount = 0;
	for (i=0; i<count; i++) {
		m_tmp = (m_tmp << 1) | (m_in->readBuf[i] > 0 ? 1 : 0);
		m_offset++;

		if (m_offset >= 8) {
			out.writeBuf[outCount] = m_tmp;
			outCount++;
			m_offset = 0;
			m_tmp = 0;
		}
	}

	m_in->flush();
	if (outCount > 0 && !out.swap(outCount)) return -1;
	return outCount;
}
