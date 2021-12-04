#pragma once

#include <dsp/block.h>

namespace dsp {
	class Packetizer : public generic_block<Packetizer> {
	public:
		Packetizer() {};
		Packetizer(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int packetLen);
		~Packetizer();

		void init(stream<uint8_t> *in, uint64_t syncWord, int syncLen, int packetLen);
		void setInput(stream<uint8_t> *in);
		int run();

		stream<uint8_t> out;
	private:
		stream<uint8_t> *_in;
		uint64_t _syncWord;
		int _syncLen, _packetLen;
		uint8_t *_rawData;
		int _dataOffset, _syncOffset;
		enum { READ, DEOFFSET } _state;

		int correlateU64(int *inverted, uint8_t *packet, int len);
	};
};
