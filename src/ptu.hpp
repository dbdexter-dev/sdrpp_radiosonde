#pragma once

#include <stdio.h>
#include <time.h>

/**
 * Wrapper around a CSV file, containing PTU data as well as time and location data
 */
class PTUWriter {
public:
	PTUWriter() { _fd = NULL; };
	~PTUWriter() { deinit(); };

	bool init(const char *fname);
	void deinit();

	/**
	 * Log a new point to file.
	 * 
	 * @param utc time of measurement
	 * @param temp temperature
	 * @param rh relative humidity
	 * @param dewpt dew point
	 * @param pressure pressure
	 * @param lat latitude
	 * @param lon longitude
	 * @param alt latitude
	 * @param spd horizontal speed
	 * @param hdg heading
	 * @param climb vertical speed
	 */
	void addPoint(time_t utc, float temp, float rh, float dewpt, float pressure, float lat, float lon, float alt, float spd, float hdg, float climb);
private:
	FILE *_fd;
};
