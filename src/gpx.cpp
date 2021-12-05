#include <stdio.h>
#include <math.h>
#include <string.h>
#include "gpx.hpp"

#define GPX_TIME_FORMAT "%Y-%m-%dT%H:%M:%SZ"

bool
GPXWriter::init(const char *fname)
{
	if (_fd) {
		terminateFile();
		fclose(_fd);
	}

	_fd = fopen(fname, "wb");
	if (!_fd) return false;;

	_trackActive = false;
	fprintf(_fd,
			"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
			"<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" version=\"1.1\" creator=\"SDR++\">\n"
	);
	_offset = ftell(_fd);
	terminateFile();
	return true;
}

void
GPXWriter::deinit()
{
	terminateFile();
	fclose(_fd);
	_fd = NULL;
}


GPXWriter::~GPXWriter()
{
	deinit();
}

void
GPXWriter::startTrack(const char *name)
{
	if (!_fd) return;
	if (_trackActive && !strcmp(name, sondeSerial)) return;

	if (_trackActive) {
		stopTrack();
	}

	strncpy(sondeSerial, name, sizeof(sondeSerial)-1);

	fseek(_fd, _offset, SEEK_SET);
	fprintf(_fd, "<trk>\n<name>%s</name>\n<trkseg>\n", name);
	_offset = ftell(_fd);
	_trackActive = true;

	terminateFile();
}


void
GPXWriter::stopTrack()
{
	if (!_fd || !_trackActive) return;
	stopTrackInternal();
	_trackActive = false;
	terminateFile();
}

void
GPXWriter::addTrackPoint(time_t time, float lat, float lon, float alt)
{
	if (!_fd) return;
	fseek(_fd, _offset, SEEK_SET);
	char timestr[sizeof("YYYY-MM-DDThh:mm:ssZ")+1];

	if (isnan(lat) || isnan(lon) || isnan(alt)) return;

	strftime(timestr, sizeof(timestr), GPX_TIME_FORMAT, gmtime(&time));
	fprintf(_fd, "<trkpt lat=\"%f\" lon=\"%f\">\n", lat, lon);
	fprintf(_fd, "<ele>%f</ele>\n", alt);
	fprintf(_fd, "<time>%s</time>\n", timestr);
	fprintf(_fd, "</trkpt>\n");
	_offset = ftell(_fd);

	terminateFile();
}

void
GPXWriter::terminateFile()
{
	unsigned long offset = _offset;
	if (!_fd) return;

	fseek(_fd, _offset, SEEK_SET);

	if (_trackActive) stopTrackInternal();

	fprintf(_fd, "</gpx>\n");
	fflush(_fd);
	_offset = offset;
}

void
GPXWriter::stopTrackInternal()
{
	if (!_fd) return;
	fseek(_fd, _offset, SEEK_SET);
	fprintf(_fd, "</trkseg>\n</trk>\n");
	_offset = ftell(_fd);

}

