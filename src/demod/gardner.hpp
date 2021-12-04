#pragma once

#include <dsp/block.h>
#include "polyphase.hpp"

namespace dsp {
	class GardnerResampler : public generic_block<GardnerResampler> {
	public:
		GardnerResampler() {};
		GardnerResampler(stream<float> *in, float symFreq, float damp, float bw, float maxFreqDelta, float targetSymFreq = 0.125);
		~GardnerResampler();

		void init(stream<float> *in, float symFreq, float damp, float bw, float maxFreqDelta, float targetSymFreq = 0.125);
		void setInput(stream<float> *in);

		int run() override;

		stream<float> out;
	private:
		stream<float> *_in;
		PolyphaseFilter _flt;
		float _alpha, _beta, _freq, _centerFreq, _maxFreqDelta;
		float _phase;
		int _state;
		float _prevSample, _interSample;
		float _avgMagnitude, _avgDC;

		void update_alpha_beta(float damp, float bw);
		int advance_timeslot();
		void retime(float sample);
		float error(float sample);
		void updateEstimate(float error);
	};
};
