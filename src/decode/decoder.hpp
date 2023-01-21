#pragma once

#include <dsp/block.h>
#include <mutex>
#include "common.hpp"
extern "C" {
#include "sondedump/include/c50.h"
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
