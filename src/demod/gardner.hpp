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
		void setLoopParams(float symFreq, float damp, float bw, float maxFreqDelta, float targetSymFreq = 0.125);

		int run() override;

		stream<float> out;
	private:
		stream<float> *m_in;
		PolyphaseFilter m_flt;
		float m_alpha, m_beta, m_freq, m_centerFreq, m_maxFreqDelta;
		float m_phase;
		int m_state;
		float m_prevSample, m_interSample;
		float m_avgMagnitude, m_avgDC;

		void update_alpha_beta(float damp, float bw);
		int advance_timeslot();
		void retime(float sample);
		float error(float sample);
		void updateEstimate(float error);
	};
};
