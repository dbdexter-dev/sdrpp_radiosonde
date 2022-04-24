#pragma once

#include <dsp/block.h>
#include <mutex>
#include "common.hpp"
extern "C" {
#include "sondedump/include/dfm09.h"
#include "sondedump/include/rs41.h"
#include "sondedump/include/ims100.h"
#include "sondedump/include/m10.h"
#include "sondedump/include/imet4.h"
}

#define LEN(x) (sizeof(x)/sizeof(*x))

static float dewpt(float temp, float rh);
static float altitude_to_pressure(float alt);

namespace radiosonde {
	template<typename T, T* (*decoder_init)(int), void (*decoder_deinit)(T*), ParserStatus (*decoder_get)(T*, SondeData*, const float*, size_t)>
	class Decoder : public dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>> {
		public:
			Decoder() {}
			~Decoder() {
				if (!dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>>::_block_init) return;
				dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>>::stop();
				dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>>::unregisterInput(m_in);
				dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>>::_block_init = false;

				decoder_deinit(m_decoder);
			}

			void init(dsp::stream<float> *in, int samplerate, void (*callback)(SondeFullData *data, void *ctx), void *ctx) {
				m_in = in;
				m_ctx = ctx;
				m_callback = callback;
				m_decoder = decoder_init(samplerate);
				m_count = m_offset = 0;

				dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>>::registerInput(m_in);
				dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>>::_block_init = true;
			}

			void deinit(void) {
				dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>>::stop();
				dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>>::unregisterInput(m_in);

				decoder_deinit(m_decoder);
			}

			int run() {
				SondeData fragment;
				int count;

				typedef dsp::generic_block<Decoder<T, decoder_init, decoder_deinit, decoder_get>> blocktype;
				assert(blocktype::_block_init);

				if ((count = m_in->read()) < 0) return -1;

				while (decoder_get(m_decoder, &fragment, m_in->readBuf, count) != PROCEED) {
					switch (fragment.type) {
						case SOURCE_END:
							return -1;
						case FRAME_END:
							m_callback(&m_data, m_ctx);
							break;
						case DATETIME:
							m_data.time = fragment.data.datetime.datetime;
							break;
						case INFO:
							m_data.serial = fragment.data.info.sonde_serial;
							m_data.seq = fragment.data.info.seq;
							m_data.burstkill = fragment.data.info.burstkill_status;
							break;
						case PTU:
							m_data.temp = fragment.data.ptu.temp;
							m_data.rh = fragment.data.ptu.rh;
							if (fragment.data.ptu.pressure > 0)
								m_data.pressure = fragment.data.ptu.pressure;
							m_data.calibrated = fragment.data.ptu.calibrated;
							m_data.calib_percent = fragment.data.ptu.calib_percent;
							m_data.dewpt = dewpt(m_data.temp, m_data.rh);
							break;
						case POSITION:
							m_data.lat = fragment.data.pos.lat;
							m_data.lon = fragment.data.pos.lon;
							m_data.alt = fragment.data.pos.alt;
							m_data.spd = fragment.data.pos.speed;
							m_data.hdg = fragment.data.pos.heading;
							m_data.climb = fragment.data.pos.climb;
							if (!(m_data.pressure == m_data.pressure) || m_data.pressure <= 0)
								m_data.pressure = altitude_to_pressure(m_data.alt);
							break;
						case XDATA:
							m_data.auxData = fragment.data.xdata.data;
							break;
						case EMPTY:
						case UNKNOWN:
						default:
							break;
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

	const float hbs[] = {0,       11000,   20000,   32000,  47000,  51000,  77000};
	const float Lbs[] = {-0.0065, 0,       0.001,   0.0028, 0.0,   -0.0028, -0.002};
	const float Pbs[] = {101325,  22632.1, 5474.89, 868.02, 110.91, 66.94,  3.96};
	const float Tbs[] = {288.15,  216.65,  216.65,  228.65, 270.65, 270.65, 214.65};

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
