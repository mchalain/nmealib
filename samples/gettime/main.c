#include <nmea/nmea.h>
#include <time.h>
#include <stdio.h>

int main()
{
	struct timespec ts;
	struct tm *tm;
#ifdef __GNUC__
	clock_gettime(CLOCK_REALTIME, &ts);
#else
	nmea_gettime(CLOCK_REALTIME, &ts);
#endif
	tm = gmtime(&ts.tv_sec);
	printf("date : %02d/%02d/%04d %02d:%02d:%02d\n",
		tm->tm_mday,
		tm->tm_mon + 1,
		tm->tm_year + 1900,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec);
	return 0;
}
