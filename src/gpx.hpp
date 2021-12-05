#pragma once

#include <stdio.h>
#include <time.h>

class GPXWriter {
public:
	GPXWriter() { _fd = NULL; };
	~GPXWriter() { deinit(); };

	bool init(const char *fname);
	void deinit();
	void startTrack(const char *name);
	void stopTrack();
	void addTrackPoint(time_t time, float lat, float lon, float alt);

private:
	void terminateFile();
	void stopTrackInternal();
	FILE *_fd;
	unsigned long _offset;
	bool _trackActive;
	char sondeSerial[64];
};
