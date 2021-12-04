#include <dsp/block.h>
#include "gardner.hpp"
#include "polyphase.hpp"

#define INTERP_FILTER_ORDER 24
#define TARGET_MAG 5
#define BIAS_POLE 0.01
#define AGC_POLE 0.001

dsp::GardnerResampler::GardnerResampler(stream<float> *in, float symFreq, float damp, float bw, float maxFreqDelta, float targetSymFreq)
{
	init(in, symFreq, damp, bw, maxFreqDelta, targetSymFreq);
}

dsp::GardnerResampler::~GardnerResampler()
{
	if (!generic_block<GardnerResampler>::_block_init) return;
	generic_block<GardnerResampler>::stop();
	generic_block<GardnerResampler>::_block_init = false;
}

void
dsp::GardnerResampler::init(stream<float> *in, float symFreq, float damp, float bw, float maxFreqDelta, float targetSymFreq)
{
	int numPhases = ceil(symFreq / targetSymFreq);  /* Oversample to get > 1/targetSymFreq samples per symbol */

	_in = in;
	_freq = _centerFreq = 2.0 * symFreq / numPhases;
	_maxFreqDelta = maxFreqDelta / numPhases;
	_phase = 0;
	_avgMagnitude = TARGET_MAG;
	_avgDC = 0;
	_state = 1;

	_flt = dsp::PolyphaseFilter(dsp::PolyphaseFilter::sincCoeffs(INTERP_FILTER_ORDER, 2.0*symFreq, numPhases), numPhases);

	update_alpha_beta(damp, bw);

	generic_block<GardnerResampler>::registerInput(_in);
	generic_block<GardnerResampler>::registerOutput(&out);
	generic_block<GardnerResampler>::_block_init = true;
}

void
dsp::GardnerResampler::setInput(stream<float>* in)
{
	generic_block<GardnerResampler>::tempStop();
	generic_block<GardnerResampler>::unregisterInput(_in);
	_in = in;
	generic_block<GardnerResampler>::registerInput(_in);
	generic_block<GardnerResampler>::tempStart();
}

int
dsp::GardnerResampler::run()
{
	float sample;
	int i, phase, count, outCount;

	assert(generic_block<GardnerResampler>::_block_init);
	if ((count = _in->read()) < 0) return -1;

	outCount = 0;
	for (i=0; i<count; i++) {
		sample = _in->readBuf[i];

		_avgDC = _avgDC * (1-BIAS_POLE) + sample * BIAS_POLE;
		sample -= _avgDC;
		/* Keep sample amplitude more or less constant */
		if (sample != 0) {
			_avgMagnitude = _avgMagnitude * (1-AGC_POLE) + fabs(sample) * AGC_POLE;
			sample *= TARGET_MAG/_avgMagnitude;
		}

		/* Feed sample to oversampling filter */
		_flt.forward(sample);

		/* Check if any of the filter phases corresponds to a slot to sample */
		for (phase = 0; phase < _flt.getNumPhases(); phase++) {
			switch (advance_timeslot()) {
				case 1:
					/* Inter-sample slot */
					_interSample = _flt.get(phase);
					break;
				case 2:
					/* Sample slot */
					sample = _flt.get(phase);
					retime(sample);
					out.writeBuf[outCount] = sample;
					outCount++;
					break;
				default:
					break;
			}
		}
	}

	_in->flush();
	if (outCount > 0 && !out.swap(outCount)) return -1;
	return outCount;
}


/* Private methods {{{*/
void
dsp::GardnerResampler::update_alpha_beta(float damp, float bw)
{
	const float denom = (1.0 + 2.0*damp*bw + bw*bw);

	_alpha = 4.0*damp*bw/denom;
	_beta = 4.0*bw*bw/denom;
}

int
dsp::GardnerResampler::advance_timeslot()
{
	int ret;

	_phase += _freq;

	if (_phase >= _state) {
		ret = _state;
		_state = (_state % 2) + 1;
		return ret;
	}

	return 0;
}

void
dsp::GardnerResampler::retime(float sample)
{
	updateEstimate(error(sample));
	_prevSample = sample;
}

float
dsp::GardnerResampler::error(float sample)
{
	return (sample - _prevSample) * _interSample;
}

void
dsp::GardnerResampler::updateEstimate(float error)
{
	float freqDelta = _freq - _centerFreq;

	_phase -= 2 - fmax(-2, fmin(2, error * _alpha));
	freqDelta += error * _beta;

	freqDelta = fmax(-_maxFreqDelta, fmin(_maxFreqDelta, freqDelta));
	_freq = _centerFreq + freqDelta;
}
/* }}} */
