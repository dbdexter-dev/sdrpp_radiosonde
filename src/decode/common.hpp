#pragma once
#include <string>

typedef struct {
	std::string serial;         /* Serial number */
	int seq;                    /* Frame sequence number */
	int burstkill;              /* Time to shutdown, -1 if inactive */
	float lat, lon, alt;        /* Latitude (degrees), longitude (degrees) altitude (meters) */
	float spd, hdg, climb;      /* Speed (m/s), heading (degrees), climb (m/s) */
	float temp, rh;             /* Temperature (degrees C), relative humidity (%) */
	float dewpt, pressure;      /* Dew point (degrees C), pressure (hPa) */
	bool calibrated;            /* Whether all the calibration data has been received */
} SondeData;
