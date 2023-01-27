#pragma once

#include <dsp/block.h>
#include <mutex>
#include "common.hpp"
extern "C" {
#include "sondedump/include/c50.h"
#include "sondedump/include/dfm09.h"
#include "sondedump/include/imet4.h"
#include "sondedump/include/ims100.h"
#include "sondedump/include/m10.h"
#include "sondedump/include/mrzn1.h"
#include "sondedump/include/rs41.h"
}

#define LEN(x) (sizeof(x)/sizeof(*x))

static float dewpt(float temp, float rh);
static float altitude_to_pressure(float alt);

namespace radiosonde {
	template<typename T, T* (*decoder_init)(int), void (*decoder_deinit)(T*), ParserStatus (*decoder_get)(T*, SondeData*, const float*, size_t)>
	class Decoder : public dsp::block {
		public:
			Decoder() {}
			~Decoder() {
				if (!dsp::block::_block_init) return;
				dsp::block::stop();
				dsp::block::unregisterInput(m_in);
				dsp::block::_block_init = false;

				decoder_deinit(m_decoder);
			}

			void init(dsp::stream<float> *in, int samplerate, void (*callback)(SondeFullData *data, void *ctx), void *ctx) {
				m_in = in;
				m_ctx = ctx;
				m_callback = callback;
				m_decoder = decoder_init(samplerate);
				m_count = m_offset = 0;

				dsp::block::registerInput(m_in);
				dsp::block::_block_init = true;
			}

			void deinit(void) {
				dsp::block::stop();
				dsp::block::unregisterInput(m_in);

				decoder_deinit(m_decoder);
			}

			int run() {
				SondeData fragment;
				int count;

				assert(dsp::block::_block_init);

				if ((count = m_in->read()) < 0) return -1;

				while (decoder_get(m_decoder, &fragment, m_in->readBuf, count) != PROCEED) {
					std::ostringstream auxStream;

					if (fragment.fields & DATA_SEQ) {
						m_data.seq = fragment.seq;
					}

					if (fragment.fields & DATA_POS) {
						m_data.lat = fragment.lat;
						m_data.lon = fragment.lon;
						m_data.alt = fragment.alt;
					}

					if (fragment.fields & DATA_SPEED) {
						m_data.spd = fragment.speed;
						m_data.hdg = fragment.heading;
						m_data.climb = fragment.climb;
					}

					if (fragment.fields & DATA_TIME) {
						m_data.time = fragment.time;
					}

					if (fragment.fields & DATA_PTU) {
						m_data.calib_percent = fragment.calib_percent;
						m_data.calibrated = m_data.calib_percent >= 100.0f;
						m_data.temp = fragment.temp;
						m_data.rh = fragment.rh;
						m_data.pressure = fragment.pressure;
						m_data.dewpt = dewpt(m_data.temp, m_data.rh);
					}

					if (fragment.fields & DATA_SERIAL) {
						m_data.serial = fragment.serial;
					}

					if (fragment.fields & DATA_SHUTDOWN) {
						m_data.burstkill = fragment.shutdown;
					}

					/* Auxiliary data */
					if (fragment.fields & DATA_OZONE) {
						auxStream.precision(2);
						auxStream << "O3=" << std::fixed << fragment.o3_mpa << "mPa";
						m_data.auxData = auxStream.str();
					}

					if (m_data.pressure <= 0) {
						m_data.pressure = altitude_to_pressure(m_data.alt);
					}

					if (fragment.fields) {
						m_callback(&m_data, m_ctx);
					}
				}

				m_in->flush();
				return 0;
			}

		private:
			dsp::stream<float> *m_in;
			void (*m_callback)(SondeFullData *data, void *ctx);
			void *m_ctx;
			T *m_decoder;
			int m_count, m_offset;
			SondeFullData m_data;

	};
}

static float
dewpt(float temp, float rh)
{
	const float tmp = (logf(rh / 100.0f) + (17.27f * temp / (237.3f + temp))) / 17.27f;
	return 237.3f * tmp  / (1 - tmp);
}
static float
altitude_to_pressure(float alt)
{
	const float g0 = 9.80665;
	const float M = 0.0289644;
	const float R_star = 8.3144598;

	const float hbs[] = {0.0,      11000.0, 20000.0, 32000.0, 47000.0, 51000.0, 77000.0};
	const float Lbs[] = {-0.0065,  0.0,     0.001,   0.0028,  0.0,     -0.0028, -0.002};
	const float Pbs[] = {101325.0, 22632.1, 5474.89, 868.02,  110.91,  66.94,   3.96};
	const float Tbs[] = {288.15,   216.65,  216.65,  228.65,  270.65,  270.65,  214.65};

	float Lb, Pb, Tb, hb;
	int b;

	for (b=0; b<(int)LEN(Lbs)-1; b++) {
		if (alt < hbs[b+1]) {
			Lb = Lbs[b];
			Pb = Pbs[b];
			Tb = Tbs[b];
			hb = hbs[b];
			break;
		}
	}

	if (b == (int)LEN(Lbs) - 1) {
		Lb = Lbs[b];
		Pb = Pbs[b];
		Tb = Tbs[b];
		hb = hbs[b];
	}

	if (Lb != 0) {
		return 1e-2 * Pb * powf((Tb + Lb * (alt - hb)) / Tb, - (g0 * M) / (R_star * Lb));
	}
	return 1e-2 * Pb * expf(-g0 * M * (alt - hb) / (R_star * Tb));
}
