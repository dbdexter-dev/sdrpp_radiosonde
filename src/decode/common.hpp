#pragma once
#include <string>

typedef struct {
	std::string serial;         /* Serial number */
	float lat, lon, alt;        /* Latitude (degrees), longitude (degrees) altitude (meters) */
	float temp, rh, pressure;   /* Temperature (degrees C), relative humidity (%), pressure (hPa) */
	bool calibrated;            /* Whether all the calibration data has been received */
} SondeData;
