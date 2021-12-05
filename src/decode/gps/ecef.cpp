#include <math.h>
#include "ecef.h"

int
ecef_to_lla(float *lat, float *lon, float *alt, float x, float y, float z)
{
	const float lambda = atan2f(y, x);
	const float p = sqrtf(x*x + y*y);
	const float theta = atan2f(z*WGS84_A,(p*WGS84_B));
	const float sintheta = sinf(theta);
	const float costheta = cosf(theta);
	float n, h, phi, sinphi;

	if (x == 0 || y == 0 || z == 0) {
		*lat = *lon = *alt = NAN;
		return 1;
	}

	phi = atan2f(z + WGS84_E_PRIME_SQR*WGS84_B*(sintheta*sintheta*sintheta),
				(p - WGS84_E_SQR*WGS84_A*(costheta*costheta*costheta)));
	sinphi = sinf(phi);
	n = WGS84_A / sqrtf(1 - WGS84_E_SQR * sinphi*sinphi);
	h = p / cosf(phi) - n;

	*lat = phi*180/M_PI;
	*lon = lambda*180/M_PI;
	*alt = h;

	return 0;
}
int
ecef_to_spd_hdg(float *speed, float *heading, float *v_climb, float lat, float lon, float dx, float dy, float dz)
{
	float v_north, v_east;
	lat *= M_PI/180;
	lon *= M_PI/180;

	if (dx == 0 && dy == 0 && dz == 0) {
		*speed = *heading = *v_climb = 0;
		return 0;
	}

	*v_climb = dx*cosf(lat)*cosf(lon) + dy*cosf(lat)*sinf(lon) + dz*sinf(lat);
	v_north = -dx*sinf(lat)*cosf(lon) - dy*sinf(lat)*sinf(lon) + dz*cosf(lat);
	v_east = -dx*sinf(lon) + dy*cosf(lon);

	*speed = sqrtf(v_north*v_north + v_east*v_east);
	*heading = atan2f(v_east, v_north) * 180/M_PI;
	if (*heading < 0) {
		*heading += 360;
	}

	return 0;
}
