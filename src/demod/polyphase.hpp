#pragma once

#include <vector>

namespace dsp {
	class PolyphaseFilter {
	public:
		PolyphaseFilter() {};
		PolyphaseFilter(std::vector<float> coeffs, unsigned numPhases);
		PolyphaseFilter(int order, float cutoff, unsigned numPhases);
		~PolyphaseFilter();

		void forward(float sample);
		float get(unsigned phase);
		int getNumPhases() { return m_numPhases; };

		static std::vector<float> sincCoeffs(int order, float cutoff, int numPhases);

	private:
		float *m_mem;
		std::vector<float> m_coeffs;
		unsigned m_size, m_numPhases, m_idx;

		static float sinc_coeff(float cutoff, int stage, unsigned taps, float osf);
	};
};
