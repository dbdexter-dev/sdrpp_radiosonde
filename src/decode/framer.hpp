#pragma once

#include <dsp/block.h>

namespace dsp {
	class Framer : public generic_block<Framer> {
	public:
		Framer() {};
		Framer(stream<uint8_t> *in, const uint64_t, int syncLen, int frameLen);
		~Framer();

		void init(stream<uint8_t> *in, const uint64_t syncWord, int syncLen, int frameLen);
		void setInput(stream<uint8_t> *in);
		void setSyncWord(uint64_t syncWord, int syncLen);
		void setFrameLen(int frameLen);
		int run();

		stream<uint8_t> out;
	private:
		stream<uint8_t> *m_in;
		uint8_t *m_syncWord;
		int m_syncLen, m_frameLen;
		uint8_t *m_rawData;
		int m_dataOffset, m_syncOffset, m_inverted;
		enum { READ, DEOFFSET } m_state;

		std::pair<int, int>correlate(uint8_t *frame);
	};
};
