#include <ctime>
#include <iostream>
#include "sondehub.hpp"
#include "httplib.h"

const std::map<std::string, std::string> SondeHubReporter::m_manufacturers = {
	{"RS41", "Vaisala"},
	{"DFM", "Graw"},
};

SondeHubReporter::SondeHubReporter(const char *callsign, const char *addr, const char *endpoint)
	: m_callsign(callsign), m_addr(addr), m_endpoint(endpoint)
{
	m_running = true;
	m_workerThread = std::thread(SondeHubReporter::workerLoop, this);
}

SondeHubReporter::~SondeHubReporter()
{
	m_running = false;
	if (m_workerThread.joinable()) {
		m_workerThread.join();
	}
}

void
SondeHubReporter::report(const SondeData &data)
{
	char datetime[64];
	auto manuf = m_manufacturers.find(data.type);
	nlohmann::json telemetry = {
		{"software_name", "sdrpp_radiosonde"},
		{"software_version", "0.5.2"},
	};

	/* Sanity checks before uploading */
	if (data.serial.length() == 0) return;
	if (manuf == m_manufacturers.end()) return;
	if (!strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%S.000000Z",
				gmtime(&data.time))) return;
	if (std::isnan(data.lat) || std::isnan(data.lon) || std::isnan(data.alt)) return;
	if (data.lat == 0 && data.lon == 0 && data.alt == 0) return;    /* -O3 -ffast-math breaks NAN */

	/* Required fields */
	telemetry["uploader_callsign"] = m_callsign;
	telemetry["type"] = data.type.c_str();
	telemetry["manufacturer"] = manuf->second;
	telemetry["serial"] = data.serial.c_str();
	telemetry["frame"] = data.seq;
	telemetry["serial"] = data.serial;
	telemetry["datetime"] = datetime;
	telemetry["lat"] = data.lat;
	telemetry["lon"] = data.lon;
	telemetry["alt"] = data.alt;

	/* Optional fields */
	telemetry["vel_h"] = data.spd;
	telemetry["vel_v"] = data.climb;
	telemetry["heading"] = data.hdg;

	if (data.calibrated) {
		telemetry["temp"] = data.temp;
		telemetry["humidity"] = data.rh;
		telemetry["pressure"] = data.pressure;
		if (data.rawAuxData.length() > 0) {
			telemetry["xdata"] = data.rawAuxData.c_str();
		}
	}

	/* Asynchronously send this to the speecified endpoint */
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_telemetryQueue.push_back(telemetry);
	}
	m_cond.notify_all();
}


void
SondeHubReporter::workerLoop(void *ctx)
{
	auto _this = (SondeHubReporter*)ctx;
	nlohmann::json element;

	while (true) {
		{
			std::unique_lock<std::mutex> lock(_this->m_mutex);
			_this->m_cond.wait(lock, [_this] { return !_this->m_running || !_this->m_telemetryQueue.empty(); });

			if (!_this->m_running) return;
			element = std::move(_this->m_telemetryQueue.back());
			_this->m_telemetryQueue.pop_back();
		}

		std::cout << element.dump(4) << std::endl;
	}
}

