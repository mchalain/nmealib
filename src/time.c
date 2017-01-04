/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: time.c 4 2007-08-27 13:11:03Z xtimor $
 *
 */

#ifdef __GNUC__
#define _GNU_SOURCE
#include <dlfcn.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <pthread.h>

#include "nmea/parse.h"
#include "nmea/parser.h"

/*! \file time.h */
#include "nmea/time.h"


#ifdef NMEA_WIN
#   pragma warning(disable: 4201)
#   pragma warning(disable: 4214)
#   pragma warning(disable: 4115)
#   include <windows.h>
#   pragma warning(default: 4201)
#   pragma warning(default: 4214)
#   pragma warning(default: 4115)
#else
#   include <time.h>
#endif

#ifdef NMEA_WIN

void nmea_time_now(nmeaTIME *stm)
{
    SYSTEMTIME st;

    GetSystemTime(&st);

    stm->year = st.wYear - 1900;
    stm->mon = st.wMonth - 1;
    stm->day = st.wDay;
    stm->hour = st.wHour;
    stm->min = st.wMinute;
    stm->sec = st.wSecond;
    stm->hsec = st.wMilliseconds / 10;
}

#else /* NMEA_WIN */

void nmea_time_now(nmeaTIME *stm)
{
    time_t lt;
    struct tm *tt;

    time(&lt);
    tt = gmtime(&lt);

    stm->year = tt->tm_year;
    stm->mon = tt->tm_mon;
    stm->day = tt->tm_mday;
    stm->hour = tt->tm_hour;
    stm->min = tt->tm_min;
    stm->sec = tt->tm_sec;
    stm->hsec = 0;
}

#endif

static int nmea_gpsfd = -1;
static nmeaPARSER nmea_parser;
static nmeaINFO nmea_info;
static struct timespec nmea_tp;
static int (*pclock_gettime)(clockid_t, struct timespec *);
static pthread_t nmea_pthread;
static pthread_attr_t nmea_attr;
static int nmea_thread_running = 0;

static void _nmea_readdatetime(nmeaINFO *info, struct timespec *tp, int satinuse);

static void *_thread_readdatetime(void *);

__attribute__((constructor)) void nmea_init()
{
	const char defaultdevicename[] = "/dev/ttyS0";
	char *devicename;
	void *launchpthread = 0;

#ifdef _GNU_SOURCE
	pclock_gettime = (int (*)(clockid_t, struct timespec *))dlsym(RTLD_NEXT, "clock_gettime");
	launchpthread = (void *)dlsym(RTLD_NEXT, "pthread_create");
#else
	pclock_gettime = clock_gettime;
#endif

	devicename = getenv("NMEA_TTYGPS");
	if (!devicename)
		devicename = (char *)defaultdevicename;

	nmea_gpsfd = open(devicename, O_RDONLY);
	if (nmea_gpsfd > 2 )
	{
		nmea_parser_init(&nmea_parser);
		nmea_zero_INFO(&nmea_info);

		if (launchpthread)
		{
			pthread_attr_init(&nmea_attr);
			pthread_attr_setdetachstate(&nmea_attr, PTHREAD_CREATE_JOINABLE);

			nmea_thread_running = 1;
			if (pthread_create(&nmea_pthread, &nmea_attr, _thread_readdatetime, NULL) != 0)
			{
				close(nmea_gpsfd);
				nmea_gpsfd = -1;
			}
		}
		else
		{
			_nmea_readdatetime(&nmea_info, &nmea_tp, 3);
		}
	}

}

__attribute__((destructor)) void nmea_deinit()
{
	if (nmea_gpsfd > 2 )
		close(nmea_gpsfd);
}

static void *_thread_readdatetime(void *unused)
{
	_nmea_readdatetime(&nmea_info, &nmea_tp, 2);
	nmea_thread_running = 0;
	return NULL;
}

static void _nmea_readdatetime(nmeaINFO *info, struct timespec *tp, int satinuse)
{
	int size = 100;
	char buff[101];

	do
	{
		size = read(nmea_gpsfd, buff, size);
		if (size > 0)
		{
			nmea_parse(&nmea_parser, &buff[0], size, info);
			if (nmea_info.satinfo.inuse > satinuse)
			{
				struct tm gpstime;

				gpstime.tm_year = nmea_info.utc.year;
				gpstime.tm_mon = nmea_info.utc.mon;
				gpstime.tm_mday = nmea_info.utc.day;
				gpstime.tm_hour = nmea_info.utc.hour;
				gpstime.tm_min = nmea_info.utc.min;
				gpstime.tm_sec = nmea_info.utc.sec;
				gpstime.tm_isdst = 0;
				/*
				 * mktime espects a local time and not UTC
				 * timezone is the global variable from <time.h>
				 */
				gpstime.tm_hour -= (timezone / 3600);
				tp->tv_sec = mktime(&gpstime);
				tp->tv_nsec = nmea_info.utc.hsec * 10000000;
			}
		}
		else if ((errno != EINTR || nmea_thread_running) && errno != EAGAIN)
		{
			close(nmea_gpsfd);
			nmea_gpsfd = -1;
			break;
		}
	} while (nmea_thread_running || !(info->smask & (GPZDA | GPGGA | GPRMC)));
}

int nmea_gettime(clockid_t clk_id, struct timespec *tp)
{
	if ((CLOCK_GPS == clk_id || CLOCK_REALTIME == clk_id) && nmea_gpsfd > 2)
	{
		if (! nmea_thread_running)
			_nmea_readdatetime(&nmea_info, &nmea_tp, 2);
		if (nmea_info.satinfo.inuse > 2)
		{
			tp->tv_sec = nmea_tp.tv_sec;
			tp->tv_nsec = nmea_tp.tv_nsec;
			nmea_info.smask &= ~(GPZDA | GPGGA | GPRMC);
			return 0;
		}
		else if (CLOCK_GPS == clk_id)
		{
			errno = EINVAL;
			return -1;
		}
	}
	return pclock_gettime(clk_id, tp);
}

#ifdef _GNU_SOURCE
int clock_gettime(clockid_t clk_id, struct timespec *tp) __attribute__ ((weak, alias ("nmea_gettime")));
#endif
