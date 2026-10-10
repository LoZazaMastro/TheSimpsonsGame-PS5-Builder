/* SPDX-License-Identifier: GPL-3.0-or-later. See the project LICENSE.
 * Keep the current SDK's single C++ destructor registry when a thread exits
 * explicitly, in addition to the SDK's normal start-routine-return path.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

extern void ps5p_run_thread_destructors(void);
extern void __real_pthread_exit(void *value) __attribute__((noreturn));
extern struct tm *ps5_gmtime_r(const time_t *value, struct tm *result);

__attribute__((visibility("hidden"), noreturn)) void
__wrap_pthread_exit(void *value)
{
    ps5p_run_thread_destructors();
    __real_pthread_exit(value);
}

__attribute__((visibility("hidden"))) int ps5_ffs(int value)
{
    return __builtin_ffs(value);
}

/* The title's libkernel exports _fpathconf, while pathconf itself belongs to
 * libkernel_sys. Query the opened object through that real native accessor;
 * retain its filesystem-specific limit, indeterminate result, and errno.
 * O_NONBLOCK prevents opening a FIFO merely to query it from waiting for I/O.
 */
#if defined(__PROSPERO__)
extern long prospero_native_fpathconf(int descriptor, int name) __asm__("_fpathconf");
#else
#define prospero_native_fpathconf fpathconf
#endif

__attribute__((visibility("hidden"))) long ps5_pathconf(const char *path, int name)
{
    if (!path) {
        errno = EFAULT;
        return -1;
    }
    const int previous_error = errno;
    const int descriptor = open(path, O_RDONLY | O_NONBLOCK);
    if (descriptor < 0) {
        return -1;
    }
    errno = previous_error;
    const long result = prospero_native_fpathconf(descriptor, name);
    const int query_error = errno;
    close(descriptor);
    errno = query_error;
    return result;
}

static int64_t floor_divide(int64_t value, int64_t divisor)
{
    const int64_t quotient = value / divisor;
    return quotient - (value % divisor < 0);
}

/* A true UTC conversion. mktime interprets its argument in the console's
 * local timezone and is not a substitute for timegm. Normalize month and
 * sub-day overflow arithmetically, then let the SDK's UTC converter publish
 * the normalized tm fields, including weekday and day of year.
 */
__attribute__((visibility("hidden"))) time_t ps5_timegm(struct tm *value)
{
    if (!value) {
        errno = EINVAL;
        return (time_t)-1;
    }
    static const int days_before_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    const int64_t carried_years = floor_divide(value->tm_mon, 12);
    const int64_t year = (int64_t)value->tm_year + 1900 + carried_years;
    const int month = (int)((int64_t)value->tm_mon - carried_years * 12);
    const int64_t previous_year = year - 1;
    int64_t days = (year - 1970) * 365;
    days += floor_divide(previous_year, 4) - 492;
    days -= floor_divide(previous_year, 100) - 19;
    days += floor_divide(previous_year, 400) - 4;
    days += days_before_month[month] + (int64_t)value->tm_mday - 1;
    if (month >= 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        ++days;
    }
    const int64_t seconds = days * 86400 + (int64_t)value->tm_hour * 3600 +
                            (int64_t)value->tm_min * 60 + value->tm_sec;
    const time_t result = (time_t)seconds;
    if ((int64_t)result != seconds) {
        errno = EOVERFLOW;
        return (time_t)-1;
    }
    struct tm normalized;
    if (!ps5_gmtime_r(&result, &normalized)) {
        return (time_t)-1;
    }
    *value = normalized;
    return result;
}
