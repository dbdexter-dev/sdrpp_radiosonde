#pragma once
#include <string>

class SondeData {
public:
	SondeData() { init(); }
	void init() {
		serial = "";
		seq = time = burstkill = 0;
		lat = lon = alt = 0;
		spd = hdg = climb = 0;
		temp = rh = dewpt = pressure = 0;
		calibrated = false;
		auxData = "";
	};

	std::string serial;         /* Serial number */
	int seq;                    /* Frame sequence number */
	time_t time;                /* Onboard time */
	int burstkill;              /* Time to shutdown, -1 if inactive */
	float lat, lon, alt;        /* Latitude (degrees), longitude (degrees) altitude (meters) */
	float spd, hdg, climb;      /* Speed (m/s), heading (degrees), climb (m/s) */
	float temp, rh;             /* Temperature (degrees C), relative humidity (%) */
	float dewpt, pressure;      /* Dew point (degrees C), pressure (hPa) */
	bool calibrated;            /* Whether all the calibration data has been received */
	std::string auxData;        /* Auxiliary freeform data */
	std::string rawAuxData;     /* Auxiliary data as a hex string */
};
