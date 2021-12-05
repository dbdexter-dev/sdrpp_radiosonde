#include <time.h>
#include "time.h"

#define GPS_EPOCH_DELTA 315964800UL
#define SECONDS_PER_WEEK (86400UL*7)

time_t
gps_time_to_utc(uint16_t week, uint32_t ms)
{
	return (time_t)(ms/1000UL)+(SECONDS_PER_WEEK*week)+GPS_EPOCH_DELTA;
}
