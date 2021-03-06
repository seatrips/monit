/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.
 */


#include "Config.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>

#include "Str.h"
#include "system/System.h"
#include "system/Time.h"


/**
 * Implementation of the Time interface
 *
 * @author http://www.tildeslash.com/
 * @see http://www.mmonit.com/
 * @file
 */


/* ----------------------------------------------------- MARK: - Definitions */


#ifndef HAVE_TIMEGM
/*
 * Spdylay - SPDY Library
 *
 * Copyright (c) 2013 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/* Counter the number of leap year in the range [0, y). The |y| is the
 year, including century (e.g., 2012) */
static int count_leap_year(int y)
{
        y -= 1;
        return y/4-y/100+y/400;
}


/* Returns nonzero if the |y| is the leap year. The |y| is the year,
 including century (e.g., 2012) */
static int is_leap_year(int y)
{
        return y%4 == 0 && (y%100 != 0 || y%400 == 0);
}


/* The number of days before ith month begins */
static int daysum[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};


/* Based on the algorithm of Python 2.7 calendar.timegm. */
time_t timegm(struct tm *tm)
{
        int days;
        int num_leap_year;
        int64_t t;
        if(tm->tm_mon > 11) {
                return -1;
        }
        num_leap_year = count_leap_year(tm->tm_year + 1900) - count_leap_year(1970);
        days = (tm->tm_year - 70) * 365 +
        num_leap_year + daysum[tm->tm_mon] + tm->tm_mday-1;
        if(tm->tm_mon >= 2 && is_leap_year(tm->tm_year + 1900)) {
                ++days;
        }
        t = ((int64_t)days * 24 + tm->tm_hour) * 3600 + tm->tm_min * 60 + tm->tm_sec;
        if(sizeof(time_t) == 4) {
                if(t < INT_MIN || t > INT_MAX) {
                        return -1;
                }
        }
        return t;
}
#endif /* !HAVE_TIMEGM */


#if HAVE_STRUCT_TM_TM_GMTOFF
#define TM_GMTOFF tm_gmtoff
#else
#define TM_GMTOFF tm_wday
#endif


#define TEST_RANGE(v, f, t) \
        do { \
                if (v < f || v > t) \
                        THROW(AssertException, "#v is outside the range <%d..%d>", f, t); \
        } while (0)
static const char days[] = "SunMonTueWedThuFriSat";
static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";


/* --------------------------------------------------------- MARK: - Private */


static inline int _a2i(const char *a, int l) {
        int n = 0;
        for (; *a && l--; a++)
                n = n * 10 + (*a - '0');
        return n;
}


/* --------------------------------------------------------- MARK: - Class */


time_t Time_toTimestamp(const char *s) {
        if (STR_DEF(s)) {
                struct tm t = {};
                if (Time_toDateTime(s, &t)) {
                        if (t.tm_year == 0)
                                THROW(AssertException, "Invalid date/time");
                        t.tm_year -= 1900;
                        time_t offset = t.TM_GMTOFF;
                        return timegm(&t) - offset;
                }
        }
        return 0;
}


struct tm *Time_toDateTime(const char *s, struct tm *t) {
        assert(t);
        assert(s);
        struct tm tm = {.tm_isdst = -1};
        int have_date = false, have_time = false;
        const char *limit = s + strlen(s), *marker, *token, *cursor = s;
        while (true) {
                if (cursor >= limit) {
                        if (have_date || have_time) {
                                *(struct tm*)t = tm;
                                return t;
                        }
                        THROW(AssertException, "Invalid date or time");
                }
                token = cursor;

                {
                        unsigned char yych;
                        unsigned int yyaccept = 0;
                        static const unsigned char yybm[] = {
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                128, 128, 128, 128, 128, 128, 128, 128,
                                128, 128,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                                0,   0,   0,   0,   0,   0,   0,   0,
                        };
                        yych = *cursor;
                        if (yych <= ',') {
                                if (yych == '+') goto yy4;
                        } else {
                                if (yych <= '-') goto yy4;
                                if (yych <= '/') goto yy2;
                                if (yych <= '9') goto yy5;
                        }
                yy2:
                        ++cursor;
                yy3:
                        {
                                continue;
                        }
                yy4:
                        yyaccept = 0;
                        yych = *(marker = ++cursor);
                        if (yych <= '/') goto yy3;
                        if (yych <= '9') goto yy6;
                        goto yy3;
                yy5:
                        yyaccept = 0;
                        yych = *(marker = ++cursor);
                        if (yych <= '/') goto yy3;
                        if (yych <= '9') goto yy8;
                        goto yy3;
                yy6:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy9;
                yy7:
                        cursor = marker;
                        if (yyaccept <= 2) {
                                if (yyaccept <= 1) {
                                        if (yyaccept == 0) {
                                                goto yy3;
                                        } else {
                                                goto yy10;
                                        }
                                } else {
                                        goto yy25;
                                }
                        } else {
                                if (yyaccept == 3) {
                                        goto yy30;
                                } else {
                                        goto yy36;
                                }
                        }
                yy8:
                        yych = *++cursor;
                        if (yych <= '/') goto yy11;
                        if (yych <= '9') goto yy12;
                        if (yych <= ':') goto yy13;
                        goto yy11;
                yy9:
                        yyaccept = 1;
                        yych = *(marker = ++cursor);
                        if (yych == '\n') goto yy10;
                        if (yych <= '/') goto yy14;
                        if (yych <= '9') goto yy15;
                        goto yy14;
                yy10:
                        { // Timezone: +-HH:MM, +-HH or +-HHMM is offset from UTC in seconds
                                if (have_time) { // Only set timezone if we have parsed time
                                        tm.TM_GMTOFF = _a2i(token + 1, 2) * 3600;
                                        if (isdigit(token[3]))
                                                tm.TM_GMTOFF += _a2i(token + 3, 2) * 60;
                                        else if (isdigit(token[4]))
                                                tm.TM_GMTOFF += _a2i(token + 4, 2) * 60;
                                        if (token[0] == '-')
                                                tm.TM_GMTOFF *= -1;
                                }
                                continue;
                        }
                yy11:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy16;
                        goto yy7;
                yy12:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy17;
                        goto yy7;
                yy13:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy18;
                        goto yy7;
                yy14:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy19;
                        goto yy7;
                yy15:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy20;
                        goto yy7;
                yy16:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy21;
                        goto yy7;
                yy17:
                        yych = *++cursor;
                        if (yych <= '/') goto yy22;
                        if (yych <= '9') goto yy23;
                        goto yy22;
                yy18:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy24;
                        goto yy7;
                yy19:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy26;
                        goto yy7;
                yy20:
                        yych = *++cursor;
                        if (yych <= '/') goto yy10;
                        if (yych <= '9') goto yy26;
                        goto yy10;
                yy21:
                        yych = *++cursor;
                        if (yych <= '/') goto yy27;
                        if (yych <= '9') goto yy7;
                        goto yy27;
                yy22:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy28;
                        goto yy7;
                yy23:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy29;
                        goto yy7;
                yy24:
                        yyaccept = 2;
                        yych = *(marker = ++cursor);
                        if (yych <= '/') goto yy27;
                        if (yych >= ':') goto yy27;
                yy25:
                        { // Time: HH:MM
                                tm.tm_hour = _a2i(token, 2);
                                tm.tm_min  = _a2i(token + 3, 2);
                                tm.tm_sec  = 0;
                                have_time = true;
                                continue;
                        }

                yy26:
                        ++cursor;
                        goto yy10;
                yy27:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy31;
                        goto yy7;
                yy28:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy32;
                        goto yy7;
                yy29:
                        yyaccept = 3;
                        yych = *(marker = ++cursor);
                        if (yych <= '-') {
                                if (yych == ',') goto yy33;
                        } else {
                                if (yych <= '.') goto yy33;
                                if (yych <= '/') goto yy30;
                                if (yych <= '9') goto yy34;
                        }
                yy30:
                        { // Compressed Time: HHMMSS
                                tm.tm_hour = _a2i(token, 2);
                                tm.tm_min  = _a2i(token + 2, 2);
                                tm.tm_sec  = _a2i(token + 4, 2);
                                have_time = true;
                                continue;
                        }
                yy31:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy35;
                        goto yy7;
                yy32:
                        yych = *++cursor;
                        if (yych <= '/') goto yy37;
                        if (yych <= '9') goto yy7;
                        goto yy37;
                yy33:
                        yych = *++cursor;
                        if (yybm[0+yych] & 128) {
                                goto yy38;
                        }
                        goto yy7;
                yy34:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy40;
                        goto yy7;
                yy35:
                        yyaccept = 4;
                        yych = *(marker = ++cursor);
                        if (yych <= '-') {
                                if (yych == ',') goto yy42;
                        } else {
                                if (yych <= '.') goto yy42;
                                if (yych <= '/') goto yy36;
                                if (yych <= '9') goto yy43;
                        }
                yy36:
                        { // Time: HH:MM:SS
                                tm.tm_hour = _a2i(token, 2);
                                tm.tm_min  = _a2i(token + 3, 2);
                                tm.tm_sec  = _a2i(token + 6, 2);
                                have_time = true;
                                continue;
                        }
                yy37:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy44;
                        goto yy7;
                yy38:
                        yych = *++cursor;
                        if (yybm[0+yych] & 128) {
                                goto yy38;
                        }
                        goto yy30;
                yy40:
                        ++cursor;
                        { // Compressed Date: YYYYMMDD
                                tm.tm_year  = _a2i(token, 4);
                                tm.tm_mon   = _a2i(token + 4, 2) - 1;
                                tm.tm_mday  = _a2i(token + 6, 2);
                                have_date = true;
                                continue;
                        }
                yy42:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy45;
                        goto yy7;
                yy43:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy47;
                        goto yy7;
                yy44:
                        yych = *++cursor;
                        if (yych <= '/') goto yy7;
                        if (yych <= '9') goto yy49;
                        goto yy7;
                yy45:
                        yych = *++cursor;
                        if (yych <= '/') goto yy36;
                        if (yych <= '9') goto yy45;
                        goto yy36;
                yy47:
                        ++cursor;
                        { // Date: dd/mm/yyyy
                                tm.tm_mday  = _a2i(token, 2);
                                tm.tm_mon   = _a2i(token + 3, 2) - 1;
                                tm.tm_year  = _a2i(token + 6, 4);
                                have_date = true;
                                continue;
                        }
                yy49:
                        ++cursor;
                        { // Date: YYYY-MM-DD
                                tm.tm_year  = _a2i(token, 4);
                                tm.tm_mon   = _a2i(token + 5, 2) - 1;
                                tm.tm_mday  = _a2i(token + 8, 2);
                                have_date = true;
                                continue;
                        }
                }
        }
        return NULL;
}


time_t Time_build(int year, int month, int day, int hour, int min, int sec) {
        struct tm tm = {.tm_isdst = -1};
        TEST_RANGE(year, 1970, 2037);
        TEST_RANGE(month, 1, 12);
        TEST_RANGE(day, 1, 31);
        TEST_RANGE(hour, 0, 23);
        TEST_RANGE(min, 0, 59);
        TEST_RANGE(sec, 0, 61);
        tm.tm_year = (year - 1900);
        tm.tm_mon  = (month - 1);
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min  = min;
        tm.tm_sec  = sec;
        return mktime(&tm);
}


time_t Time_now(void) {
	struct timeval t;
	if (gettimeofday(&t, NULL) != 0)
                THROW(AssertException, "%s", System_getLastError());
	return t.tv_sec;
}


int64_t Time_milli(void) {
	struct timeval t;
	if (gettimeofday(&t, NULL) != 0)
                THROW(AssertException, "%s", System_getLastError());
	return (int64_t)t.tv_sec * 1000  +  (int64_t)t.tv_usec / 1000;
}


int64_t Time_micro(void) {
	struct timeval t;
	if (gettimeofday(&t, NULL) != 0)
                THROW(AssertException, "%s", System_getLastError());
	return (int64_t)t.tv_sec * 1000000  +  (int64_t)t.tv_usec;
}


int Time_seconds(time_t time) {
        struct tm tm;
        localtime_r(&time, &tm);
        return tm.tm_sec;
}


int Time_minutes(time_t time) {
        struct tm tm;
        localtime_r(&time, &tm);
        return tm.tm_min;
}


int Time_hour(time_t time) {
        struct tm tm;
        localtime_r(&time, &tm);
        return tm.tm_hour;
}


int Time_weekday(time_t time) {
        struct tm tm;
        localtime_r(&time, &tm);
        return tm.tm_wday;
}


int Time_day(time_t time) {
        struct tm tm;
        localtime_r(&time, &tm);
        return tm.tm_mday;
}


int Time_month(time_t time) {
        struct tm tm;
        localtime_r(&time, &tm);
        return (tm.tm_mon + 1);
}


int Time_year(time_t time) {
        struct tm tm;
        localtime_r(&time, &tm);
        return (tm.tm_year + 1900);
}


#define i2a(i) (x[0]=(i/10)+'0', x[1]=(i%10)+'0')
char *Time_string(time_t time, char *result) {
        if (result) {
                char x[2];
                struct tm ts;
                localtime_r((const time_t *)&time, &ts);
                memcpy(result, "aaa, xx aaa xxxx xx:xx:xx\0", 26);
                /*              0    5  8   1214 17 20 2326 */
                memcpy(result, days + 3 * ts.tm_wday, 3);
                i2a(ts.tm_mday);
                result[5] = x[0];
                result[6] = x[1];
                memcpy(result + 8, months + 3 * ts.tm_mon, 3);
                i2a((ts.tm_year + 1900) / 100);
                result[12] = x[0];
                result[13] = x[1];
                i2a((ts.tm_year + 1900) % 100);
                result[14] = x[0];
                result[15] = x[1];
                i2a(ts.tm_hour);
                result[17] = x[0];
                result[18] = x[1];
                i2a(ts.tm_min);
                result[20] = x[0];
                result[21] = x[1];
                i2a(ts.tm_sec);
                result[23] = x[0];
                result[24] = x[1];
        }
	return result;
}


char *Time_gmtstring(time_t time, char *result) {
        if (result) {
                char x[2];
                struct tm ts;
                gmtime_r(&time, &ts);
                memcpy(result, "aaa, xx aaa xxxx xx:xx:xx GMT\0", 30);
                /*              0    5  8   1214 17 20 23    29 */
                memcpy(result, days + 3 * ts.tm_wday, 3);
                i2a(ts.tm_mday);
                result[5] = x[0];
                result[6] = x[1];
                memcpy(result + 8, months + 3 * ts.tm_mon, 3);
                i2a((ts.tm_year + 1900) / 100);
                result[12] = x[0];
                result[13] = x[1];
                i2a((ts.tm_year + 1900) % 100);
                result[14] = x[0];
                result[15] = x[1];
                i2a(ts.tm_hour);
                result[17] = x[0];
                result[18] = x[1];
                i2a(ts.tm_min);
                result[20] = x[0];
                result[21] = x[1];
                i2a(ts.tm_sec);
                result[23] = x[0];
                result[24] = x[1];
        }
	return result;
}


char *Time_fmt(char *result, int size, const char *format, time_t time) {
        struct tm tm;
        assert(result);
        assert(format);
        localtime_r((const time_t *)&time, &tm);
        if (strftime(result, size, format, &tm) == 0)
                *result = 0;
        return result;
}


char *Time_uptime(time_t sec, char *result) {
        // Write max 24 bytes to result
        if (result) {
                int n = 0;
                time_t r = 0;
                result[0] = 0;
                if (sec > 0) {
                        if ((r = sec/86400) > 0) {
                                n = snprintf(result, 24, "%lldd", (int64_t)r);
                                sec -= r * 86400;
                        }
                        if ((r = sec/3600) > 0) {
                                n += snprintf(result + n, (24 - n), "%s%lldh", n ? ", " : "", (int64_t)r);
                                sec -= r * 3600;
                        }
                        r = sec/60;
                        snprintf(result + n, (24 - n), "%s%lldm", n ? ", " : "", (int64_t)r);
                }
        }
        return result;
}


/*
 cron string is on format "minute hour day month wday"
 where fields may have a numeric type, an asterix, a
 sequence of numbers or a range
 */
int Time_incron(const char *cron, time_t time) {
        assert(cron);
#define YYCURSOR cron
#define YYLIMIT  end
#define YYTOKEN  t
        const char *m;
        const char *t;
        const char *end = cron + strlen(cron);
        int n = 0;
        int found = 0;
        int fields[] = {Time_minutes(time), Time_hour(time), Time_day(time), Time_month(time), Time_weekday(time)};

parse:
        if (YYCURSOR >= YYLIMIT)
                return found == 5;
        YYTOKEN = YYCURSOR;

        {
                unsigned char yych;
                static const unsigned char yybm[] = {
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        128, 128, 128, 128, 128, 128, 128, 128,
                        128, 128,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                        0,   0,   0,   0,   0,   0,   0,   0,
                };

                yych = *cron;
                if (yych <= ' ') {
                        if (yych <= '\f') {
                                if (yych <= 0x08) goto yy10;
                                if (yych >= '\v') goto yy10;
                        } else {
                                if (yych <= '\r') goto yy2;
                                if (yych <= 0x1F) goto yy10;
                        }
                } else {
                        if (yych <= '+') {
                                if (yych == '*') goto yy4;
                                goto yy10;
                        } else {
                                if (yych <= ',') goto yy6;
                                if (yych <= '/') goto yy10;
                                if (yych <= '9') goto yy8;
                                goto yy10;
                        }
                }
        yy2:
                ++cron;
                {
                        goto parse;
                }
        yy4:
                ++cron;
                {
                        n++;
                        found++;
                        goto parse;
                }
        yy6:
                ++cron;
                {
                        n--; // backtrack on fields advance
                        assert(n < 5 && n >= 0);
                        goto parse;
                }
        yy8:
                yych = *(m = ++cron);
                goto yy13;
        yy9:
                {
                        if (fields[n] == Str_parseInt(YYTOKEN))
                                found++;
                        n++;
                        goto parse;
                }
        yy10:
                ++cron;
                {
                        return false;
                }
        yy12:
                m = ++cron;
                yych = *cron;
        yy13:
                if (yybm[0+yych] & 128) {
                        goto yy12;
                }
                if (yych != '-') goto yy9;
                yych = *++cron;
                if (yych <= '/') goto yy15;
                if (yych <= '9') goto yy16;
        yy15:
                cron = m;
                goto yy9;
        yy16:
                ++cron;
                yych = *cron;
                if (yych <= '/') goto yy18;
                if (yych <= '9') goto yy16;
        yy18:
                {
                        int from = Str_parseInt(YYTOKEN);
                        int to = Str_parseInt(strchr(YYTOKEN, '-') + 1);
                        if ((fields[n] <= to) && (fields[n] >= from))
                                found++;
                        n++;
                        goto parse;
                }
        }

        return found == 5;
}


void Time_usleep(long u) {
#ifdef NETBSD
        // usleep is broken on NetBSD (at least in version 5.1)
        struct timespec t = {u / 1000000, (u % 1000000) * 1000};
        nanosleep(&t, NULL);
#else
        usleep((useconds_t)u);
#endif
}

