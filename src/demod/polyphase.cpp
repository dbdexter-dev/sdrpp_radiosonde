#include <assert.h>
#include <math.h>
#include "polyphase.hpp"

dsp::PolyphaseFilter::PolyphaseFilter(std::vector<float> coeffs, unsigned numPhases)
{
	m_size = ceilf(coeffs.size() / numPhases);
	m_numPhases = numPhases;
	m_coeffs = coeffs;
	m_mem = new float[coeffs.size() / numPhases];
	m_idx = 0;
}

dsp::PolyphaseFilter::~PolyphaseFilter()
{
	delete m_mem;
}

void
dsp::PolyphaseFilter::forward(float sample) {
	m_mem[m_idx++] = sample;
	m_idx %= m_size;
}

float
dsp::PolyphaseFilter::get(unsigned phase) {
	int i, j;
	float result;

	assert(phase < m_numPhases);

	result = 0;
	j = m_size * (m_numPhases - phase - 1);

	/* Chunk 1: from current position to end */
	for (i = m_idx; i<m_size; i++, j++) {
		result += m_mem[i] * m_coeffs[j];
	}
	/* Chunk 2: from start to current position - 1 */
	for (i = 0; i < m_idx; i++, j++) {
		result += m_mem[i] * m_coeffs[j];
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
		}
	}

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
