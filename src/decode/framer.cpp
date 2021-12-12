#include "framer.hpp"
extern "C" {
#include "utils.h"
}

#define CEILDIV(x, y) (((x)+((y)-1))/(y))

dsp::Framer::Framer(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int frameLen)
{
	init(in, syncWord, syncLen, frameLen);
}

dsp::Framer::~Framer()
{
	if (!generic_block<Framer>::_block_init) return;

	delete[] m_syncWord;
	delete[] m_rawData;

	generic_block<Framer>::stop();
	generic_block<Framer>::unregisterInput(m_in);
	generic_block<Framer>::unregisterOutput(&out);
	generic_block<Framer>::_block_init = false;
}

void
dsp::Framer::init(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int frameLen)
{
	m_in = in;
	m_syncWord = new uint8_t[syncLen];
	for (int i=0; i<syncLen; i++) {
		m_syncWord[i] = (syncWord >> (syncLen - i - 1)) & 0x01;
	}
	m_syncLen = syncLen;

	m_frameLen = frameLen;
	m_rawData = new uint8_t[2*frameLen];
	m_state = READ;
	m_dataOffset = 0;

	generic_block<Framer>::registerInput(m_in);
	generic_block<Framer>::registerOutput(&out);
	generic_block<Framer>::_block_init = true;
}

void
dsp::Framer::setInput(stream<uint8_t> *in)
{
	generic_block<Framer>::tempStop();
	generic_block<Framer>::unregisterInput(m_in);

	m_in = in;
	m_state = READ;
	m_dataOffset = 0;

	generic_block<Framer>::registerInput(m_in);
	generic_block<Framer>::tempStart();
}

void
dsp::Framer::setSyncWord(uint64_t syncWord, int syncLen)
{
	delete[] m_syncWord;
	m_syncWord = new uint8_t[syncLen];
	for (int i=0; i<syncLen; i++) {
		m_syncWord[i] = (syncWord >> (syncLen - i - 1)) & 0x01;
	}
	m_syncLen = syncLen;
}

void
dsp::Framer::setFrameLen(int frameLen)
{
	delete[] m_rawData;
	m_rawData = new uint8_t[2 * frameLen];
	m_frameLen = frameLen;
	m_dataOffset = 0;
	m_state = READ;
}

int
dsp::Framer::run()
{
	int i, bitOffset, numBytes, count, outCount;
	int chunkSize;
	std::pair<int, int> bestCorr;
	uint8_t *src;

	assert(generic_block<Framer>::_block_init);
	if ((count = m_in->read()) < 0) return -1;

	src = m_in->readBuf;

	outCount = 0;
	while (count > 0) {
		switch (m_state) {
			case READ:
				/* Try to read a frame worth of bits */
				numBytes = std::min(m_frameLen - m_dataOffset, count);
				memcpy(m_rawData + m_dataOffset, src, numBytes);

				count -= numBytes;
				m_dataOffset += numBytes;
				src += numBytes;

				/* If an entire frame is not available, return */
				if (count <= 0) {
					break;
				}

				/* Find offset with the highest correlation */
				bestCorr = correlate(m_rawData);
				m_inverted = std::get<1>(bestCorr);
				m_dataOffset = m_frameLen - std::get<0>(bestCorr);
				memcpy(m_rawData, m_rawData + std::get<0>(bestCorr), m_dataOffset);
				m_state = DEOFFSET;
				break;

			case DEOFFSET:

				/* Try to read enough bits to undo the offset */
				numBytes = std::min(m_frameLen - m_dataOffset, count);
				memcpy(m_rawData + m_dataOffset, src, numBytes);

				src += numBytes;
				count -= numBytes;
				m_dataOffset += numBytes;

				/* If an entire frame is not available, return */
				if (count <= 0) {
					break;
				}

				/* Copy bits into a new frame */
				memcpy(out.writeBuf + outCount, m_rawData, m_frameLen);
				outCount += m_frameLen;
				if (m_inverted) {
					for (i=0; i<m_frameLen; i++) {
						out.writeBuf[i] ^= 0x01;
					}
				}

				/* Get ready for the next read */
				m_dataOffset = 0;
				m_state = READ;
				break;

			default:
				m_state = READ;
				break;

		}
	}

	m_in->flush();
	if (outCount > 0 && !out.swap(outCount)) return -1;
	return outCount;
}


/* Private methods {{{ */
std::pair<int, int>
dsp::Framer::correlate(uint8_t *frame)
{
	int inverted;
	int corr, bestCorr, bestOffset;
	uint8_t tmp;

	bestOffset = 0;
	bestCorr = hamming_memcmp(frame, m_syncWord, m_syncLen);

	/* If the syncword is found at offset 0, return immediately */
	if (bestCorr == 0) return std::pair<int, int>(0, 0);

	/* Search for the position with the highest correlation */
	for (int i=0; i < m_frameLen - m_syncLen; i++) {

		/* Check correlation with syncword */
		corr = hamming_memcmp(frame + i, m_syncWord, m_syncLen);
		if (corr < bestCorr) {
			bestCorr = corr;
			bestOffset = i;
			inverted = 0;
		}

		/* Check correlation with inverted syncword */
		corr = m_syncLen - corr;
		if (corr < bestCorr) {
			bestCorr = corr;
			bestOffset = i;
			inverted = 1;
		}
	}

	return std::pair<int, int>(bestOffset, inverted);
}
/* }}} */
