#include <iostream>
#include "framer.hpp"
#include "utils.h"

#define CEILDIV(x, y) (((x)+((y)-1))/(y))

static inline int inverseCorrelateU64(uint64_t x, uint64_t y);
static void bitcpy(uint8_t *dst, uint8_t *src, size_t offset, size_t bits);

dsp::Framer::Framer(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int frameLen)
{
	init(in, syncWord, syncLen, frameLen);
}

dsp::Framer::~Framer()
{
	if (!generic_block<Framer>::_block_init) return;
	delete m_rawData;
	generic_block<Framer>::stop();
	generic_block<Framer>::_block_init = false;
}

void
dsp::Framer::init(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int frameLen)
{
	m_in = in;
	m_syncWord = syncWord;
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
	m_syncWord = syncWord;
	m_syncLen = syncLen;
}

void
dsp::Framer::setFrameLen(int frameLen)
{
	delete m_rawData;
	m_rawData = new uint8_t[2*frameLen];
	m_frameLen = frameLen;
	m_dataOffset = 0;
	m_state = READ;
}

int
dsp::Framer::run()
{
	int i, bitOffset, inverted, numBytes, count, outCount;
	int chunkSize;
	uint8_t *src;

	if ((count = m_in->read()) < 0) return -1;

	src = m_in->readBuf;

	outCount = 0;
	while (count > 0) {
		switch (m_state) {

			case READ:
				/* Try to read a frame worth of bits */
				numBytes = std::min(m_frameLen - m_dataOffset/8, count);
				memcpy(m_rawData + CEILDIV(m_dataOffset, 8), src, numBytes);

				if (m_dataOffset % 8) {
					bitpack(m_rawData + m_dataOffset/8, m_rawData+CEILDIV(m_dataOffset, 8), m_dataOffset%8, numBytes*8);
				}

				count -= numBytes;
				m_dataOffset += 8*numBytes;
				src += numBytes;

				/* If an entire frame is not available, return */
				if (count <= 0) {
					m_in->flush();
					if (outCount > 0 && !out.swap(outCount)) return -1;
					return outCount;
				}

				/* Find offset with the highest correlation */
				m_syncOffset = correlateU64(&inverted, m_rawData, m_frameLen);
				m_state = DEOFFSET;
				__attribute__((fallthrough));

			case DEOFFSET:

				/* Try to read enough bits to undo the offset */
				numBytes = std::min(m_frameLen - (m_dataOffset-m_syncOffset)/8, count);
				memcpy(m_rawData + CEILDIV(m_dataOffset, 8), src, numBytes);

				if (m_dataOffset % 8) {
					bitpack(m_rawData + m_dataOffset/8, m_rawData+CEILDIV(m_dataOffset, 8), m_dataOffset%8, numBytes*8);
				}
				src += numBytes;
				count -= numBytes;
				m_dataOffset += 8*numBytes;

				/* If an entire frame is not available, return */
				if (count <= 0) {
					m_in->flush();
					if (outCount > 0 && !out.swap(outCount)) return -1;
					return outCount;
				}

				/* Copy bits into a new frame */
				bitcpy(m_rawData, m_rawData, m_syncOffset, 8*m_frameLen);
				for (i=0; i<m_frameLen; i++) {
					out.writeBuf[outCount++] = inverted ? 0xFF ^m_rawData[i] : m_rawData[i];
				}

				/* If the offset is not byte-aligned, copy the last bits to the
				 * beginning of the new frame */
				bitcpy(m_rawData, m_rawData, m_syncOffset, 8);
				m_dataOffset = m_syncOffset%8;
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
int
dsp::Framer::correlateU64(int *inverted, uint8_t *frame, int len)
{
	int i, j;
	int corr, bestCorr, bestOffset;
	const uint64_t syncMask = (m_syncLen < 8) ? ((1ULL << (8*m_syncLen))) : ~0ULL;
	const uint64_t syncWord = m_syncWord & syncMask;
	uint64_t window;
	uint8_t tmp;

	window = 0;
	for (i=0; i<m_syncLen; i++) {
		window = window << 8 | *frame++;
	}

	bestOffset = 0;
	bestCorr = inverseCorrelateU64(syncWord, window & syncMask);

	/* If the syncword is found at offset 0, return immediately */
	if (bestCorr == 0) return 0;


	/* Search for the position with the highest correlation */
	for (i=0; i<len - m_syncLen; i++) {
		tmp = *frame++;

		/* For each bit in the byte */
		for (j=0; j<8; j++) {

			/* Check correlation with syncword */
			corr = inverseCorrelateU64(syncWord, window & syncMask);
			if (corr < bestCorr) {
				bestCorr = corr;
				bestOffset = i*8+j;
				*inverted = 0;
			}

			/* Check correlation with inverted syncword */
			corr = m_syncLen*8 - corr;
			if (corr < bestCorr) {
				bestCorr = corr;
				bestOffset = i*8+j;
				*inverted = 1;
			}

			/* Advance window by one */
			window = ((window << 1) | ((tmp >> (7-j)) & 0x1));
		}

	}

	return bestOffset;
}

/**
 * Count the number of bits that differ between two uint64's
 */
static inline int
inverseCorrelateU64(uint64_t x, uint64_t y)
{
	int corr;
	uint64_t v = x ^ y;

	/* From bit twiddling hacks */
	for (corr = 0; v; corr++) {
		v &= v-1;
	}

	return corr;
}

/* }}} */
