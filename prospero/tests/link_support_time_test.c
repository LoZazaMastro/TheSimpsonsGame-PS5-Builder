/* Host checks for the link layer's missing libc functions. */
#define _GNU_SOURCE 1
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

extern time_t ps5_timegm(struct tm *value);
extern int ps5_ffs(int value);

struct tm *ps5_gmtime_r(const time_t *value, struct tm *result)
{
    return gmtime_r(value, result);
}
void ps5p_run_thread_destructors(void) {}
__attribute__((noreturn)) void __real_pthread_exit(void *unused)
{
    (void)unused;
    abort();
}

int main(void)
{
    /* Local timezone and DST settings must not affect UTC conversion. */
    const char *zones[] = {"UTC0", "EST5EDT", "JST-9"};
    for (unsigned zone = 0; zone < sizeof(zones) / sizeof(zones[0]); ++zone) {
        setenv("TZ", zones[zone], 1);
        tzset();
        struct tm epoch = {.tm_year = 70, .tm_mon = 0, .tm_mday = 1, .tm_isdst = 1};
        assert(ps5_timegm(&epoch) == 0);
        assert(epoch.tm_wday == 4 && epoch.tm_yday == 0 && epoch.tm_isdst == 0);
        struct tm leap = {.tm_year = 100, .tm_mon = 1, .tm_mday = 29};
        assert(ps5_timegm(&leap) == 951782400);
        struct tm overflow = {.tm_year = 69, .tm_mon = 12, .tm_mday = 0, .tm_sec = 86400};
        assert(ps5_timegm(&overflow) == 0);
        assert(overflow.tm_year == 70 && overflow.tm_mon == 0 && overflow.tm_mday == 1);
        struct tm underflow = {.tm_year = 71, .tm_mon = -12, .tm_mday = 1, .tm_sec = -1};
        errno = 0;
        assert(ps5_timegm(&underflow) == -1 && errno == 0);
        assert(underflow.tm_year == 69 && underflow.tm_mon == 11 && underflow.tm_mday == 31);
    }
    /* Compare normalized UTC round trips over leap years and the epoch. */
    for (time_t seconds = -2208988800LL; seconds < 4133980800LL; seconds += 1234567) {
        struct tm date;
        assert(gmtime_r(&seconds, &date));
        assert(ps5_timegm(&date) == seconds);
    }
    assert(ps5_ffs(0) == 0);
    assert(ps5_ffs(1) == 1);
    assert(ps5_ffs(0x8000) == 16);
    assert(ps5_ffs((int)0x80000000u) == 32);
    assert(ps5_ffs(-1) == 1);
}
