#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "gpx.hpp"

#define GPX_TIME_FORMAT "%Y-%m-%dT%H:%M:%SZ"

bool
GPXWriter::init(const char *fname)
{
	if (m_fd) deinit();

	m_fd = fopen(fname, "wb");
	if (!m_fd) return false;

	m_lat = m_lon = m_alt = m_time = 0;

	m_trackActive = false;
	fprintf(m_fd,
			"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
			"<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" version=\"1.1\" creator=\"SDR++\">\n"
	);
	m_offset = ftell(m_fd);
	terminateFile();
	return true;
}

void
GPXWriter::deinit()
{
	if (!m_fd) return;
	terminateFile();
	fclose(m_fd);
	m_fd = NULL;
}

void
GPXWriter::startTrack(const char *name)
{
	if (!m_fd) return;
	if (m_trackActive && !strcmp(name, sondeSerial)) return;
	for (int i=0; name[i] != '\0'; i++) if (!isgraph(name[i])) return;

	if (m_trackActive) {
		stopTrack();
	}

	strncpy(sondeSerial, name, sizeof(sondeSerial)-1);

	fseek(m_fd, m_offset, SEEK_SET);
	fprintf(m_fd, "<trk>\n<name>%s</name>\n<trkseg>\n", name);
	m_offset = ftell(m_fd);
	m_trackActive = true;

	terminateFile();
}


void
GPXWriter::stopTrack()
{
	if (!m_fd || !m_trackActive) return;
	stopTrackInternal();
	m_trackActive = false;
	terminateFile();
}

void
GPXWriter::addTrackPoint(time_t time, float lat, float lon, float alt, float spd, float hdg)
{
	if (!m_fd || !m_trackActive) return;
	fseek(m_fd, m_offset, SEEK_SET);
	char timestr[sizeof("YYYY-MM-DDThh:mm:ssZ")+1];

	if (isnan(lat) || isnan(lon) || isnan(alt)) return;
	if (lat == 0 && lon == 0 && alt == 0) return;       /* -ffast-math breaks NaN */
	if (time == m_time || (lat == m_lat && lon == m_lon && alt == m_alt)) return;

	m_lat = lat;
	m_lon = lon;
	m_alt = alt;
	m_time = time;

	strftime(timestr, sizeof(timestr), GPX_TIME_FORMAT, gmtime(&time));
	fprintf(m_fd, "<trkpt lat=\"%f\" lon=\"%f\">\n", lat, lon);
	fprintf(m_fd, "<time>%s</time>\n", timestr);
	fprintf(m_fd, "<ele>%f</ele>\n", alt);
	fprintf(m_fd, "<speed>%f</speed>\n", spd);
	fprintf(m_fd, "<course>%f</course>\n", hdg);
	fprintf(m_fd, "</trkpt>\n");
	m_offset = ftell(m_fd);

	terminateFile();
}

void
GPXWriter::terminateFile()
{
	unsigned long offset = m_offset;
	if (!m_fd) return;

	fseek(m_fd, m_offset, SEEK_SET);

	if (m_trackActive) stopTrackInternal();

	fprintf(m_fd, "</gpx>\n");
	fflush(m_fd);
	m_offset = offset;
}

void
GPXWriter::stopTrackInternal()
{
	if (!m_fd) return;
	fseek(m_fd, m_offset, SEEK_SET);
	fprintf(m_fd, "</trkseg>\n</trk>\n");
	m_offset = ftell(m_fd);

}

