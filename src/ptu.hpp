#pragma once

#include <stdio.h>
#include <time.h>

class PTUWriter {
public:
	PTUWriter() { _fd = NULL; };
	~PTUWriter() { deinit(); };

	bool init(const char *fname);
	void deinit();
	void addPoint(time_t utc, float temp, float rh, float dewpt, float pressure, float alt, float spd, float hdg);
private:
	FILE *_fd;
};
