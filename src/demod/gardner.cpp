#include <dsp/block.h>
#include "gardner.hpp"
#include "polyphase.hpp"

#define INTERP_FILTER_ORDER 24
#define TARGET_MAG 5
#define BIAS_POLE 0.01
#define AGC_POLE 0.001

dsp::GardnerResampler::~GardnerResampler()
{
	if (!generic_block<GardnerResampler>::_block_init) return;

	generic_block<GardnerResampler>::stop();
	generic_block<GardnerResampler>::unregisterInput(m_in);
	generic_block<GardnerResampler>::unregisterOutput(&out);
	generic_block<GardnerResampler>::_block_init = false;
}

void
dsp::GardnerResampler::init(stream<float> *in, float symFreq, float damp, float bw, float maxFreqDelta, float targetSymFreq)
{
	int numPhases = ceil(symFreq / targetSymFreq);  /* Oversample to get > 1/targetSymFreq samples per symbol */

	m_in = in;
	m_freq = m_centerFreq = 2.0 * symFreq / numPhases;
	m_maxFreqDelta = maxFreqDelta / numPhases;
	m_phase = 0;
	m_avgMagnitude = TARGET_MAG;
	m_avgDC = 0;
	m_state = 1;
	update_alpha_beta(damp, bw);

	m_flt = dsp::PolyphaseFilter(dsp::PolyphaseFilter::sincCoeffs(INTERP_FILTER_ORDER, 2.0*symFreq, numPhases), numPhases);


	generic_block<GardnerResampler>::registerInput(m_in);
	generic_block<GardnerResampler>::registerOutput(&out);
	generic_block<GardnerResampler>::_block_init = true;
}

void
dsp::GardnerResampler::setInput(stream<float>* in)
{
	generic_block<GardnerResampler>::tempStop();
	generic_block<GardnerResampler>::unregisterInput(m_in);
	m_in = in;
	generic_block<GardnerResampler>::registerInput(m_in);
	generic_block<GardnerResampler>::tempStart();
}

void
dsp::GardnerResampler::setLoopParams(float symFreq, float damp, float bw, float maxFreqDelta, float targetSymFreq)
{
	int numPhases = ceil(symFreq / targetSymFreq);  /* Oversample to get > 1/targetSymFreq samples per symbol */

	m_freq = m_centerFreq = 2.0 * symFreq / numPhases;
	m_maxFreqDelta = maxFreqDelta / numPhases;
	update_alpha_beta(damp, bw);

	m_flt = dsp::PolyphaseFilter(dsp::PolyphaseFilter::sincCoeffs(INTERP_FILTER_ORDER, 2.0*symFreq, numPhases), numPhases);
}

int
dsp::GardnerResampler::run()
{
	float sample;
	int i, phase, count, outCount;

	assert(generic_block<GardnerResampler>::_block_init);
	if ((count = m_in->read()) < 0) return -1;

	outCount = 0;
	for (i=0; i<count; i++) {
		sample = m_in->readBuf[i];

		m_avgDC = m_avgDC * (1-BIAS_POLE) + sample * BIAS_POLE;
		sample -= m_avgDC;
		/* Keep sample amplitude more or less constant */
		if (sample != 0) {
			m_avgMagnitude = m_avgMagnitude * (1-AGC_POLE) + fabs(sample) * AGC_POLE;
			sample *= TARGET_MAG/m_avgMagnitude;
		}

		/* Feed sample to oversampling filter */
		m_flt.forward(sample);

		/* Check if any of the filter phases corresponds to a slot to sample */
		for (phase = 0; phase < m_flt.getNumPhases(); phase++) {
			switch (advance_timeslot()) {
				case 1:
					/* Inter-sample slot */
					m_interSample = m_flt.get(phase);
					break;
				case 2:
					/* Sample slot */
					sample = m_flt.get(phase);
					retime(sample);
					out.writeBuf[outCount] = sample;
					outCount++;
					break;
				default:
					break;
			}
		}
	}

	m_in->flush();
	if (outCount > 0 && !out.swap(outCount)) return -1;
	return outCount;
}


/* Private methods {{{*/
void
dsp::GardnerResampler::update_alpha_beta(float damp, float bw)
{
	const float denom = (1.0 + 2.0*damp*bw + bw*bw);

	m_alpha = 4.0*damp*bw/denom;
	m_beta = 4.0*bw*bw/denom;
}

int
dsp::GardnerResampler::advance_timeslot()
{
	int ret;

	m_phase += m_freq;

	if (m_phase >= m_state) {
		ret = m_state;
		m_state = (m_state % 2) + 1;
		return ret;
	}

	return 0;
}

void
dsp::GardnerResampler::retime(float sample)
{
	updateEstimate(error(sample));
	m_prevSample = sample;
}

float
dsp::GardnerResampler::error(float sample)
{
	/* If no transition, don't attempt any correction */
	if (!((sample > 0) ^ (m_prevSample > 0))) return 0;
	return (sample - m_prevSample) * m_interSample;
}

void
dsp::GardnerResampler::updateEstimate(float error)
{
	float freqDelta = m_freq - m_centerFreq;

	m_phase -= 2 - fmax(-2, fmin(2, error * m_alpha));
	freqDelta += error * m_beta;

	freqDelta = fmax(-m_maxFreqDelta, fmin(m_maxFreqDelta, freqDelta));
	m_freq = m_centerFreq + freqDelta;
}
/* }}} */
