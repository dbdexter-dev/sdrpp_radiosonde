#pragma once
#include "decode/common.hpp"

class TelemetryReporter {
public:
	virtual ~TelemetryReporter() {};

	virtual void report(const SondeData &data) = 0;
	virtual const char *getType() = 0;
	virtual const char *getTarget() = 0;
	virtual const char *getCallsign() = 0;
	virtual const char *getStatus() = 0;
};
