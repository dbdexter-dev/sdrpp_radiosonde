#pragma once
#include "decode/common.hpp"

class TelemetryReporter {
public:
	virtual void report(const SondeData &data) = 0;
};
