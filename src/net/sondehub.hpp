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
	~SondeHubReporter();

	void report(const SondeData &data);
private:
	const static std::map<std::string, std::string> m_manufacturers;
	std::string m_callsign;
	std::string m_addr;
	std::string m_endpoint;
	std::thread m_workerThread;
	std::atomic<bool> m_running;
	std::mutex m_mutex;
	std::condition_variable m_cond;
	std::vector<nlohmann::json> m_telemetryQueue;

	static void workerLoop(void *ctx);
};
