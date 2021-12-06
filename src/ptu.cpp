#include "ptu.hpp"

bool
PTUWriter::init(const char *fname)
{
	if (_fd) deinit();

	_fd = fopen(fname, "wb");
	if(!_fd) return false;

	fprintf(_fd, "Epoch,Temperature,Relative humidity,Dew point,Pressure,Latitude,Longitude,Altitude,Speed,Heading,Climb\n");

	return true;
}

void
PTUWriter::deinit()
{
	if (!_fd) return;
	fclose(_fd);
	_fd = NULL;
}

void
PTUWriter::addPoint(time_t utc, float temp, float rh, float dewpt, float pressure, float lat, float lon, float alt, float spd, float hdg, float climb)
{
	if (!_fd) return;
	fprintf(_fd, "%ld,%.1f,%.1f,%.1f,%.1f,%.6f,%.6f,%.1f,%.1f,%.1f,%.1f\n", utc, temp, rh, dewpt, pressure, lat, lon, alt, spd, hdg, climb);
	fflush(_fd);
}
