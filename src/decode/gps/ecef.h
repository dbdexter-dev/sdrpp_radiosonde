#pragma once

#define WGS84_A 6378137.0
#define WGS84_F (1 / 298.257223563)
#define WGS84_B (WGS84_A * (1 - WGS84_F))
#define WGS84_E_SQR ((WGS84_A*WGS84_A - WGS84_B*WGS84_B)/(WGS84_A*WGS84_A))
#define WGS84_E (sqrtf(WGS84_E_SQR))
#define WGS84_E_PRIME_SQR ((WGS84_A*WGS84_A - WGS84_B*WGS84_B)/(WGS84_B*WGS84_B))
#define WGS84_E_PRIME (sqrtf(WGS84_E_PRIME))

/**
 * Convert ECEF coordinates to lat/lon/alt
 *
 * @param lat pointer latitude, in degrees
 * @param lon pointer to longitude, in degrees
 * @param alt pointer to altitude, in meters
 * @param x ECEF x coordinate
 * @param y ECEF y coordinate
 * @param z ECEF z coordinate
 *
 * @return 0 on success, 1 on invalid xyz coordinates
 */
int ecef_to_lla(float *lat, float *lon, float *alt, float x, float y, float z);

/**
 * Convert ECEF velocity vector to speed/heading/climb rate
 *
 * @param speed pointer to speed, in m/s
 * @param heading pointer to heading, in degrees (0-360)
 * @param v_climb pointer to vertical climb, in m/s
 * @param lat latitude, in degrees
 * @param lon longitude, in degrees
 * @param dx ECEF velocity x component
 * @param dy ECEF velocity y component
 * @param dz ECEF velocity z component
 */
int ecef_to_spd_hdg(float *speed, float *heading, float *v_climb, float lat, float lon, float dx, float dy, float dz);
