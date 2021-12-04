#pragma once

#include <dsp/block.h>

namespace dsp {
	class Framer : public generic_block<Framer> {
	public:
		Framer() {};
		Framer(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int frameLen);
		~Framer();

		void init(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int frameLen);
		void setInput(stream<uint8_t> *in);
		int run();

		stream<uint8_t> out;
	private:
		stream<uint8_t> *_in;
		uint64_t _syncWord;
		int _syncLen, _frameLen;
		uint8_t *_rawData;
		int _dataOffset, _syncOffset;
		enum { READ, DEOFFSET } _state;

		int correlateU64(int *inverted, uint8_t *frame, int len);
	};
};
