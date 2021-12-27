#include <sstream>
#include <iomanip>
#include "decoder.hpp"
#include "decode/gps/ecef.h"
#include "decode/gps/time.h"
extern "C" {
#include "utils.h"
#include "dfm09.h"
}

static FILE *debug;

static time_t my_timegm(struct tm *tm);

DFM09Decoder::DFM09Decoder(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx)
{
	init(in, handler, ctx);
}

DFM09Decoder::~DFM09Decoder()
{
	if (!generic_block<DFM09Decoder>::_block_init) return;

	generic_block<DFM09Decoder>::unregisterInput(m_in);
	generic_block<DFM09Decoder>::_block_init = false;
}

void
DFM09Decoder::init(dsp::stream<uint8_t> *in, void (*handler)(SondeData *data, void *ctx), void *ctx)
{
	m_in = in;
	m_ctx = ctx;
	m_handler = handler;
	m_nullCh = -1;
	m_serialBitmask = 0;
	m_serial = 0;
	m_sondeData.init("DFM");

	debug = fopen("/tmp/debug.data", "wb");

	memset(&m_gpsTime, '\0', sizeof(m_gpsTime));

	generic_block<DFM09Decoder>::registerInput(m_in);
	generic_block<DFM09Decoder>::_block_init = true;
}

void
DFM09Decoder::setInput(dsp::stream<uint8_t>* in)
{
	generic_block<DFM09Decoder>::tempStop();
	generic_block<DFM09Decoder>::unregisterInput(m_in);

	m_in = in;
	m_sondeData.init("DFM");

	generic_block<DFM09Decoder>::registerInput(m_in);
	generic_block<DFM09Decoder>::tempStart();
}

void
DFM09Decoder::doStop()
{
	m_serialBitmask = 0;
	m_sondeData.init("DFM");
	generic_block<DFM09Decoder>::doStop();
}

int
DFM09Decoder::run()
{
	DFM09Frame frame;
	DFM09ParsedFrame parsedFrame;
	int count;

	assert(generic_block<DFM09Decoder>::_block_init);

	if ((count = m_in->read()) < 0) return -1;

	dfm09_manchester_decode(&frame, m_in->readBuf);
	dfm09_deinterleave(&frame);

	auto errcount = dfm09_correct(&frame);
	if (errcount < 0 || errcount > 1) {
		m_in->flush();
		return 0;
	}

	dfm09_unpack(&parsedFrame, &frame);

	parsePTUSubframe(&parsedFrame.ptu);
	if (parsedFrame.gps[0].type == 0x00) m_handler(&m_sondeData, m_ctx);
	parseGPSSubframe(&parsedFrame.gps[0]);
	if (parsedFrame.gps[1].type == 0x00) m_handler(&m_sondeData, m_ctx);
	parseGPSSubframe(&parsedFrame.gps[1]);

	m_in->flush();
	return 0;
}

void
DFM09Decoder::parsePTUSubframe(DFM09Subframe_PTU *ptu)
{
	const uint32_t data = ((int)(ptu->data[0]) << 16) | ((int)(ptu->data[1]) << 8) | ptu->data[2];
	std::ostringstream ss;

	if (ptu->type > 2) {
		printf("%02x\t", ptu->type);
		for (int i=0; i<3; i++) printf("%02x", ptu->data[i]);
		printf("\n");
	}

	if (data == 0) {
		/* Null channel marker */
		m_nullCh = ptu->type;
		return;
	}


	if (m_nullCh > 0 && ptu->type > m_nullCh) {
		/* Serial number */
		auto idx = data & 0xF;
		auto serial = ((data >> 4) & 0xFFFF);
		m_serialBitmask |= 1 << idx;
		m_serial |= serial << (16 * (1-idx));
		if (m_serialBitmask == 0x3) {
			ss << "D" << std::setw(8) << std::setfill('0') << std::hex << std::uppercase;
			ss << m_serial;
			m_sondeData.serial = ss.str();
		}
	} else {
		switch (ptu->type) {
			case 0x00:
				/* Temperature? */
				break;
			case 0x01:
				/* Humidity? */
				break;
			case 0x02:
				/* Pressure? */
				break;
			default:
				break;
		}
	}
}

void
DFM09Decoder::parseGPSSubframe(DFM09Subframe_GPS *gps)
{
	uint64_t tmp;

	switch (gps->type) {
		case 0x00:  /* Frame number */
			m_sondeData.seq = bitmerge(gps->data, 32);
			break;
		case 0x01:  /* GPS time? */
			m_gpsTime.tm_sec = bitmerge(gps->data + 4, 16) / 1000;
			break;
		case 0x02:  /* Lat + dLat? */
			m_sondeData.lat = (int32_t)bitmerge(gps->data, 32) / 1e7;
			m_sondeData.spd = bitmerge(gps->data+4, 16) / 1e2;
			break;
		case 0x03:  /* Lon + dLon? */
			m_sondeData.lon = (int32_t)bitmerge(gps->data, 32) / 1e7;
			m_sondeData.hdg = bitmerge(gps->data+4, 16) / 1e2;
			break;
		case 0x04:
			m_sondeData.alt = (int32_t)bitmerge(gps->data, 32) / 1e2;
			m_sondeData.climb = (int16_t)bitmerge(gps->data+4, 16) / 1e2;
			m_sondeData.pressure = altitude_to_pressure(m_sondeData.alt);
			break;
		case 0x05:
			/* Unknown */
			break;
		case 0x06:
			/* Unknown */
			break;
		case 0x07:
			/* Unknown */
			break;
		case 0x08: /* GPS date */
			tmp = bitmerge(gps->data, 32);

			m_gpsTime.tm_year = (tmp >> (32 - 12) & 0xFFF) - 1900;
			m_gpsTime.tm_mon = tmp >> (32 - 16) & 0xF;
			m_gpsTime.tm_mday = tmp >> (32 - 21) & 0x1F;
			m_gpsTime.tm_hour = tmp >> (32 - 26) & 0x1F;
			m_gpsTime.tm_min = tmp & 0x3F;

			m_sondeData.time = timegm(&m_gpsTime);
			break;
		default:
			break;
	}
}

static time_t
my_timegm(struct tm *tm)
{
    time_t ret;
    char *tz;

    tz = getenv("TZ");
    if (tz) tz = strdup(tz);
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz) {
        setenv("TZ", tz, 1);
        free(tz);
    } else {
        unsetenv("TZ");
	}
    tzset();
    return ret;
}
