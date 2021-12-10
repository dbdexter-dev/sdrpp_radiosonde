#pragma once

#include <stdint.h>
#define CCITT_FALSE_INIT 0xFFFF
#define CCITT_FALSE_POLY 0x1021

/* CRC16 checksum */
uint16_t crc16(uint16_t poly, uint16_t init, uint8_t *data, int len);

/* Misaligned bit copy (source is misaligned) */
void bitcpy(uint8_t *dst, const uint8_t *src, int offset, int bits);

/* Misaligned bit copy (destination is misaligned) */
void bitpack(uint8_t *dst, const uint8_t *src, int offset, int bits);

/* Merge bits into a single uint */
uint64_t bitmerge(uint8_t *data, int nbits);

/**
 * Calculate pressure at a given altitude
 *
 * @param alt altitude
 * @return pressure at that altitude (hPa)
 */
float altitude_to_pressure(float alt);
float pressure_to_altitude(float p);

/**
 * Calculate dew point
 *
 * @param temp temperature (degrees Celsius)
 * @param rh relative humidity (0-100%)
 * @return dew point (degrees Celsius)
 */
float dewpt(float temp, float rh);

/**
 * Calculate saturation mixing ratio given temperature and pressure
 *
 * @param temp temperature (degrees Celsius)
 * @param p pressure (hPa)
 * @return saturation mixing ratio (g/kg)
 */
float sat_mixing_ratio(float temp, float p);

/**
 * Calculate water vapor saturation pressure at the given temperature
 *
 * @param temp temperatuer (degrees Celsius)
 * @return saturation pressure (hPa)
 */
float wv_sat_pressure(float temp);

