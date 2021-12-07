#include "ptu.hpp"

bool
PTUWriter::init(const char *fname)
{
	if (m_fd) deinit();

	m_fd = fopen(fname, "wb");
	if(!m_fd) return false;

	fprintf(m_fd, "Epoch,Temperature,Relative humidity,Dew point,Pressure,Latitude,Longitude,Altitude,Speed,Heading,Climb\n");

	return true;
}

void
PTUWriter::deinit()
{
	if (!m_fd) return;
	fclose(m_fd);
	m_fd = NULL;
}

void
PTUWriter::addPoint(time_t utc, float temp, float rh, float dewpt, float pressure, float lat, float lon, float alt, float spd, float hdg, float climb)
{
	if (!m_fd) return;
	fprintf(m_fd, "%ld,%.1f,%.1f,%.1f,%.1f,%.6f,%.6f,%.1f,%.1f,%.1f,%.1f\n", utc, temp, rh, dewpt, pressure, lat, lon, alt, spd, hdg, climb);
	fflush(m_fd);
}
