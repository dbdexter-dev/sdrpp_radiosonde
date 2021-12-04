#include <iostream>
#include "polyphase.hpp"

dsp::PolyphaseFilter::PolyphaseFilter(std::vector<float> coeffs, unsigned numPhases) : _mem(coeffs.size() / numPhases)
{
	_size = ceil(coeffs.size() / numPhases);
	_numPhases = numPhases;
	_coeffs = coeffs;
	_idx = 0;
}

dsp::PolyphaseFilter::~PolyphaseFilter()
{
}

void
dsp::PolyphaseFilter::forward(float sample) {
	_mem[_idx++] = sample;
	_idx %= _size;
}

float
dsp::PolyphaseFilter::get(unsigned phase) {
	int i, j;
	float result;

	assert(phase < _numPhases);

	result = 0;
	j = _size * (_numPhases - phase - 1);

	/* Chunk 1: from current position to end */
	for (i = _idx; i<_size; i++, j++) {
		result += _mem[i] * _coeffs[j];
	}
	/* Chunk 2: from start to current position - 1 */
	for (i = 0; i < _idx; i++, j++) {
		result += _mem[i] * _coeffs[j];
	}

	return result;
}

std::vector<float>
dsp::PolyphaseFilter::sincCoeffs(int order, float cutoff, int numPhases)
{
	int i, j;
	const int taps = order * 2 + 1;
	std::vector<float> coeffs(taps*numPhases, 0.0f);


	for (j=0; j<numPhases; j++) {
		for (i=0; i<taps; i++) {
			coeffs[j*taps + i] = sinc_coeff(cutoff/numPhases, i*numPhases + j, taps*numPhases, numPhases);
			std::cout << coeffs[j*taps + i] << " ";
		}
	}
	std::cout << std::endl;

	return coeffs;
}


float
dsp::PolyphaseFilter::sinc_coeff(float cutoff, int stage_no, unsigned taps, float osf)
{
	const float norm = 2.0/5.0;
	float sinc_coeff, hamming_coeff;
	float t;
	int order;

	order = (taps - 1) / 2;

	if (order == stage_no) {
		return norm;
	}

	t = abs(order - stage_no) / osf;

	/* Sinc coefficient */
	sinc_coeff = sinf(2*M_PI*t*cutoff)/(2*M_PI*t*cutoff);

	/* Hamming windowing function */
	hamming_coeff = 0.42
		- 0.5*cosf(2*M_PI*stage_no/(taps-1))
		+ 0.08*cosf(4*M_PI*stage_no/(taps-1));

	return norm * sinc_coeff * hamming_coeff;
}
