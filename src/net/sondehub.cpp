#include <ctime>
#include <iostream>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "sondehub.hpp"
#include "httplib.h"
#include "version.h"

const std::map<std::string, std::string> SondeHubReporter::m_manufacturers = {
	{"RS41", "Vaisala"},
	{"DFM", "Graw"},
};

SondeHubReporter::SondeHubReporter(const char *callsign, const char *addr, const char *endpoint)
	: m_callsign(callsign), m_addr(addr), m_endpoint(endpoint)
{
	m_running = true;
	m_uploaderPosition = false;
	m_workerThread = std::thread(SondeHubReporter::workerLoop, this);
}

SondeHubReporter::SondeHubReporter(const char *callsign, const char *addr, const char *endpoint, float lat, float lon, float alt)
	: m_callsign(callsign), m_addr(addr), m_endpoint(endpoint)
{
	m_running = true;
	m_uploaderPosition = true;
	m_lat = lat;
	m_lon = lon;
	m_alt = alt;
	m_workerThread = std::thread(SondeHubReporter::workerLoop, this);
}

SondeHubReporter::~SondeHubReporter()
{
	m_running = false;
	m_cond.notify_all();
	if (m_workerThread.joinable()) {
		m_workerThread.join();
	}
}

void
SondeHubReporter::setPosition(float lat, float lon, float alt)
{
	m_lat = lat;
	m_lon = lon;
	m_alt = alt;
	m_uploaderPosition = true;
}

void
SondeHubReporter::setCallsign(const char *callsign)
{
	m_callsign = callsign;
}

void
SondeHubReporter::report(const SondeData &data)
{
	char datetime[64];
	auto manuf = m_manufacturers.find(data.type);
	nlohmann::json telemetry = {
		{"software_name", "sdrpp_radiosonde"},
		{"software_version", RS_VERSION},
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

	if (m_uploaderPosition) {
		telemetry["uploader_position"] = {m_lat, m_lon, m_alt};
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
	time_t now;
	char dateHeader[128];
	nlohmann::json element;

	setlocale(LC_TIME, "C");

	while (true) {
		{
			std::unique_lock<std::mutex> lock(_this->m_mutex);
			_this->m_cond.wait(lock, [_this] { return !_this->m_running || !_this->m_telemetryQueue.empty(); });

			if (!_this->m_running) return;
			element = std::move(_this->m_telemetryQueue.back());
			/* Only report the last telemetry frame */
			_this->m_telemetryQueue.clear();
			//_this->m_telemetryQueue.pop_back();
		}

		now = time(NULL);
		strftime(dateHeader, sizeof(dateHeader),
				"%a, %d %b %Y %T GMT",
				gmtime(&now));
		httplib::Client client(_this->m_addr.c_str());
		httplib::Headers headers = {
			{"Date", dateHeader}
		};

		try {
			auto resp = client.Put(_this->m_endpoint.c_str(), headers, element.dump(4), "application/json");
			if (resp.error() != httplib::Error::Success) {
				spdlog::warn("[sdrpp_radiosonde] HTTP connection failed: {0}", resp.error());
				_this->m_status = "Error";
			} else if (resp->status != 200) {
				spdlog::warn("[sdrpp_radiosonde] PUT request failed ({0}): {1}", resp->status, resp->body);
				_this->m_status = "Error";
			} else {
				_this->m_status = resp->body;
			}
		} catch (std::exception &e) {
			spdlog::warn("[sdrpp_radiosonde] Error uploading telemetry: {0}", e.what());
			_this->m_status = e.what();
		}

	}
}

