#pragma once

#include <dsp/block.h>

namespace dsp {
	class PolyphaseFilter {
	public:
		PolyphaseFilter() {};
		PolyphaseFilter(std::vector<float> coeffs, unsigned numPhases);
		PolyphaseFilter(int order, float cutoff, unsigned numPhases);
		~PolyphaseFilter();

		void forward(float sample);
		float get(unsigned phase);
		static std::vector<float> sincCoeffs(int order, float cutoff, int numPhases);
		int getNumPhases() { return _numPhases; };

	private:
		std::vector<float> _mem;
		std::vector<float> _coeffs;
		unsigned _size, _numPhases, _idx;

		static float sinc_coeff(float cutoff, int stage, unsigned taps, float osf);
	};
};
