#include <iostream>
#include "packetizer.hpp"
#include "utils.h"

#define CEILDIV(x, y) (((x)+((y)-1))/(y))

static inline int inverseCorrelateU64(uint64_t x, uint64_t y);
static void bitcpy(uint8_t *dst, uint8_t *src, size_t offset, size_t bits);

dsp::Packetizer::Packetizer(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int packetLen)
{
	init(in, syncWord, syncLen, packetLen);
}

dsp::Packetizer::~Packetizer()
{
	if (!generic_block<Packetizer>::_block_init) return;
	generic_block<Packetizer>::stop();
	generic_block<Packetizer>::_block_init = false;
}

void
dsp::Packetizer::init(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int packetLen)
{
	_in = in;
	_syncWord = syncWord;
	_syncLen = syncLen;
	_packetLen = packetLen;
	_rawData = new uint8_t[2*packetLen];
	_state = READ;
	_dataOffset = 0;

	generic_block<Packetizer>::registerInput(_in);
	generic_block<Packetizer>::registerOutput(&out);
	generic_block<Packetizer>::_block_init = true;
}

void
dsp::Packetizer::setInput(stream<uint8_t> *in)
{
	generic_block<Packetizer>::tempStop();
	generic_block<Packetizer>::unregisterInput(_in);
	_in = in;
	_state = READ;
	_dataOffset = 0;
	generic_block<Packetizer>::registerInput(_in);
	generic_block<Packetizer>::tempStart();
}

int
dsp::Packetizer::run()
{
	int i, bitOffset, inverted, numBytes, count, outCount;
	int chunkSize;
	uint8_t *src;

	if ((count = _in->read()) < 0) return -1;

	src = _in->readBuf;

	outCount = 0;
	while (count > 0) {
		switch (_state) {

			case READ:
				/* Try to read a packet worth of bits */
				numBytes = std::min(_packetLen - _dataOffset/8, count);
				memcpy(_rawData + CEILDIV(_dataOffset, 8), src, numBytes);

				if (_dataOffset % 8) {
					bitpack(_rawData + _dataOffset/8, _rawData+CEILDIV(_dataOffset, 8), _dataOffset%8, numBytes*8);
				}

				count -= numBytes;
				_dataOffset += 8*numBytes;
				src += numBytes;

				/* If an entire packet is not available, return */
				if (count <= 0) {
					_in->flush();
					if (outCount > 0 && !out.swap(outCount)) return -1;
					return outCount;
				}

				/* Find offset with the highest correlation */
				_syncOffset = correlateU64(&inverted, _rawData, _packetLen);
				_state = DEOFFSET;
				__attribute__((fallthrough));

			case DEOFFSET:

				/* Try to read enough bits to undo the offset */
				numBytes = std::min(_packetLen - (_dataOffset-_syncOffset)/8, count);
				memcpy(_rawData + CEILDIV(_dataOffset, 8), src, numBytes);

				if (_dataOffset % 8) {
					bitpack(_rawData + _dataOffset/8, _rawData+CEILDIV(_dataOffset, 8), _dataOffset%8, numBytes*8);
				}
				src += numBytes;
				count -= numBytes;
				_dataOffset += 8*numBytes;

				/* If an entire packet is not available, return */
				if (count <= 0) {
					_in->flush();
					if (outCount > 0 && !out.swap(outCount)) return -1;
					return outCount;
				}

				/* Copy bits into a new packet */
				bitcpy(_rawData, _rawData, _syncOffset, 8*_packetLen);
				for (i=0; i<_packetLen; i++) {
					out.writeBuf[outCount++] = _rawData[i];
				}

				/* If the offset is not byte-aligned, copy the last bits to the
				 * beginning of the new packet */
				bitcpy(_rawData, _rawData, _syncOffset, 8);
				_dataOffset = _syncOffset%8;
				_state = READ;
				break;

			default:
				_state = READ;
				break;

		}
	}

	_in->flush();
	if (outCount > 0 && !out.swap(outCount)) return -1;
	return outCount;
}


/* Private methods {{{ */
int
dsp::Packetizer::correlateU64(int *inverted, uint8_t *packet, int len)
{
	int i, j;
	int corr, bestCorr, bestOffset;
	const uint64_t syncMask = (_syncLen < 8) ? ((1ULL << (8*_syncLen))) : ~0ULL;
	const uint64_t syncWord = _syncWord & syncMask;
	uint64_t window;
	uint8_t tmp;

	window = 0;
	for (i=0; i<_syncLen; i++) {
		window = window << 8 | *packet++;
	}

	bestOffset = 0;
	bestCorr = inverseCorrelateU64(syncWord, window & syncMask);

	/* If the syncword is found at offset 0, return immediately */
	if (bestCorr == 0) return 0;


	/* Search for the position with the highest correlation */
	for (i=0; i<len - _syncLen; i++) {
		tmp = *packet++;

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
			corr = _syncLen*8 - corr;
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
