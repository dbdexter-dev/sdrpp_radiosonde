#include "ptu.hpp"

bool
PTUWriter::init(const char *fname)
{
	if (m_fd) deinit();

	m_fd = fopen(fname, "wb");
	if(!m_fd) return false;

	fprintf(m_fd, "Epoch,Temperature,Relative humidity,Dew point,Pressure,Latitude,Longitude,Altitude,Speed,Heading,Climb,XDATA\n");

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
PTUWriter::addPoint(SondeFullData *data)
{
	if (!m_fd) return;
	fprintf(m_fd, "%ld,%.1f,%.1f,%.1f,%.1f,%.6f,%.6f,%.1f,%.1f,%.1f,%.1f,%s\n",
			data->time,
			data->temp, data->rh, data->dewpt, data->pressure,
			data->lat, data->lon, data->alt,
			data->spd, data->hdg, data->climb,
			data->auxData.c_str());
	fflush(m_fd);
}
