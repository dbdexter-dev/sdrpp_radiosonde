#pragma once
#include <atomic>
#include <condition_variable>
#include <map>
#include <thread>
#include <json.hpp>
#include "common.hpp"

class SondeHubReporter : public TelemetryReporter {
public:
	SondeHubReporter(const char *callsign, const char *addr, const char *endpoint);
	SondeHubReporter(const char *callsign, const char *addr, const char *endpoint, float lat, float lon, float alt);
	~SondeHubReporter();

	void setCallsign(const char *callsign);
	void setPosition(float lat, float lon, float alt);

	void report(const SondeData &data);
	const char *getType() { return "SondeHub"; };
	const char *getTarget() { return m_addr.c_str(); };
	const char *getCallsign()  { return m_callsign.c_str(); };
	const char *getStatus() { return m_status.c_str(); };
private:
	const static std::map<std::string, std::string> m_manufacturers;
	float m_lat, m_lon, m_alt;
	bool m_uploaderPosition;
	std::string m_callsign;
	std::string m_addr;
	std::string m_endpoint;
	std::string m_status;
	std::thread m_workerThread;
	std::atomic<bool> m_running;
	std::mutex m_mutex;
	std::condition_variable m_cond;
	std::vector<nlohmann::json> m_telemetryQueue;

	static void workerLoop(void *ctx);
};
