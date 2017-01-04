#include <time.h>
#include <stdio.h>

#ifdef __GNUC__
#define _GNU_SOURCE
#endif

#ifndef _GNU_SOURCE
#include <nmea/nmea.h>
#endif

int main()
{
	struct timespec ts;
	struct tm *tm;

	do
	{
#ifdef _GNU_SOURCE
	clock_gettime(CLOCK_REALTIME, &ts);
#else
	nmea_gettime(CLOCK_GPS, &ts);
#endif
	tm = gmtime(&ts.tv_sec);
	printf("date : %02d/%02d/%04d %02d:%02d:%02d %d\n",
		tm->tm_mday,
		tm->tm_mon + 1,
		tm->tm_year + 1900,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec,
		ts.tv_nsec);
	sleep(1);
	} while(1);
	return 0;
}
