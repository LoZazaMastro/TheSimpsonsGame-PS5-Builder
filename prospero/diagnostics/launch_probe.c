/*
 * Optional PPSA74207 diagnostic; no installation or runtime-management calls.
 * Launch ABI adapted from Sonic tools/controllers/launch.c:
 * Copyright (C) 2026 BlackBearReloaded. SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Kernel snapshots use the existing, read-only ShadowMountPlus HTTP operation.
 * This payload does not elevate credentials or call private kernel log APIs.
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define TARGET_TITLE "PPSA74207"
#ifdef LAUNCH_PROBE_HOST_TEST_DIR
#if !defined(__linux__)
#error "The test directory override is only for the Linux host tests."
#endif
#define TARGET_DIRECTORY LAUNCH_PROBE_HOST_TEST_DIR
#else
#define TARGET_DIRECTORY "/data/homebrew/PPSA74207"
#endif
#define REPORT_PATH TARGET_DIRECTORY "/logs/launch-report.log"
#define LOG_DIRECTORY TARGET_DIRECTORY "/logs"
#define KERNEL_TAIL_BYTES 131072u
/* JSON can escape one byte as six ASCII bytes; also allow HTTP headers. */
#define HTTP_CAPACITY (6u * KERNEL_TAIL_BYTES + 8192u)
#define HTTP_TIMEOUT_MS 5000

typedef struct {
    uint32_t size;
    int32_t user_id;
    uint32_t app_opt;
    uint64_t crash_report;
    uint32_t check_flag;
} LaunchAppParam;
_Static_assert(sizeof(LaunchAppParam) == 32, "Unexpected launch parameter ABI");

int sceUserServiceInitialize(void *);
int sceUserServiceGetForegroundUser(int32_t *);
int sceSystemServiceLaunchApp(const char *, const char **, LaunchAppParam *);

static FILE *report;
static bool write_failed;

static void note(const char *format, ...) {
    va_list args, console_args;
    va_start(args, format);
    va_copy(console_args, args);
    if (vfprintf(report, format, args) < 0)
        write_failed = true;
    (void)vprintf(format, console_args);
    va_end(console_args);
    va_end(args);
}

static bool persist_report(void) {
    if (write_failed || fflush(report) != 0 || fsync(fileno(report)) != 0) {
        write_failed = true;
        (void)fprintf(stderr, "Launch probe: report write/flush failed: %s\n",
                      strerror(errno));
        return false;
    }
    return true;
}

static int open_report_directory(const char *path) {
    /* Match the package's writable-state permissions only for directories
     * created by this invocation. Existing directory permissions stay intact. */
    bool created = mkdir(path, 0777) == 0;
    if (!created && errno != EEXIST)
        return -1;
    int fd = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (fd < 0)
        return -1;
    if (created && fchmod(fd, 0777) != 0) {
        int saved_errno = errno;
        (void)close(fd);
        errno = saved_errno;
        return -1;
    }
    return fd;
}

static FILE *open_report(void) {
    /* Only these directories and this append-only report may be created.
     * The SDK's raw *at stubs do not translate carry/errno: all filesystem
     * calls here use the real imported POSIX entry points instead. */
    int root_fd = open_report_directory(TARGET_DIRECTORY);
    if (root_fd < 0)
        return NULL;
    int logs_fd = open_report_directory(LOG_DIRECTORY), fd = -1;
    if (logs_fd >= 0)
        fd = open(REPORT_PATH,
                  O_WRONLY | O_CREAT | O_APPEND | O_NOFOLLOW | O_CLOEXEC, 0644);
    int saved_errno = errno;
    if (logs_fd >= 0)
        (void)close(logs_fd);
    (void)close(root_fd);
    if (fd < 0) {
        errno = saved_errno;
        return NULL;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_nlink != 1) {
        (void)close(fd);
        errno = EINVAL;
        return NULL;
    }
    FILE *file = fdopen(fd, "a");
    if (!file)
        (void)close(fd);
    return file;
}

static int64_t monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return -1;
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static bool wait_socket(int fd, short events, int64_t deadline) {
    for (;;) {
        int64_t now = monotonic_ms();
        if (now < 0)
            return false;
        if (now >= deadline) {
            errno = ETIMEDOUT;
            return false;
        }
        struct pollfd pfd = { .fd = fd, .events = events, .revents = 0 };
        int rc = poll(&pfd, 1, (int)(deadline - now));
        if (rc < 0 && errno == EINTR)
            continue;
        if (rc <= 0) {
            if (rc == 0)
                errno = ETIMEDOUT;
            return false;
        }
        if (pfd.revents & POLLNVAL) {
            errno = EBADF;
            return false;
        }
        if (pfd.revents & (events | POLLERR | POLLHUP))
            return true;
    }
}

static bool kernel_http(char *buffer, size_t *length) {
    /* This is the local listener observed in the user's ShadowMountPlus log. */
    static const char body[] = "{\"max_bytes\":131072}";
    char request[320];
    int size = snprintf(request, sizeof(request),
        "POST /api/v1/kernel-log HTTP/1.1\r\n"
        "Host: 127.0.0.1:10101\r\nContent-Type: application/json\r\n"
        "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
        sizeof(body) - 1u, body);
    if (size <= 0 || (size_t)size >= sizeof(request)) {
        errno = EOVERFLOW;
        return false;
    }
    int64_t start = monotonic_ms();
    if (start < 0)
        return false;
    int64_t deadline = start + HTTP_TIMEOUT_MS;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return false;
    bool ok = false;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        goto done;
    struct sockaddr_in address = {0};
#if defined(__FreeBSD__) || defined(__PROSPERO__) || defined(__PS5__)
    address.sin_len = sizeof(address);
#endif
    address.sin_family = AF_INET;
    address.sin_port = htons(10101);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        if (errno != EINPROGRESS || !wait_socket(fd, POLLOUT, deadline))
            goto done;
        int socket_error = 0;
        socklen_t option_size = sizeof(socket_error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &option_size) != 0)
            goto done;
        if (socket_error != 0) {
            errno = socket_error;
            goto done;
        }
    }
    size_t sent = 0;
    while (sent < (size_t)size) {
        if (!wait_socket(fd, POLLOUT, deadline))
            goto done;
        ssize_t count = send(fd, request + sent, (size_t)size - sent, MSG_NOSIGNAL);
        if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
            continue;
        if (count <= 0) {
            if (count == 0)
                errno = EPIPE;
            goto done;
        }
        sent += (size_t)count;
    }
    *length = 0;
    while (*length < HTTP_CAPACITY) {
        if (!wait_socket(fd, POLLIN, deadline))
            goto done;
        ssize_t count = recv(fd, buffer + *length, HTTP_CAPACITY - *length, 0);
        if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
            continue;
        if (count < 0)
            goto done;
        if (count == 0) {
            buffer[*length] = '\0';
            ok = true;
            goto done;
        }
        *length += (size_t)count;
    }
    errno = EOVERFLOW;
done: {
    int saved_errno = errno;
    (void)close(fd);
    errno = saved_errno;
    return ok;
    }
}

static void skip_space(const char **p, const char *end) {
    while (*p < end && isspace((unsigned char)**p))
        ++*p;
}

/* Validate JSON strings without changing the snapshot's original escaping. */
static bool json_string(const char **p, const char *end,
                        const char **start, size_t *length) {
    if (*p >= end || *(*p)++ != '"')
        return false;
    *start = *p;
    while (*p < end) {
        unsigned char ch = (unsigned char)*(*p)++;
        if (ch == '"') {
            *length = (size_t)(*p - *start - 1);
            return true;
        }
        if (ch < 0x20)
            return false;
        if (ch == '\\') {
            if (*p >= end)
                return false;
            ch = (unsigned char)*(*p)++;
            if (ch == 'u') {
                for (int i = 0; i < 4; ++i)
                    if (*p >= end || !isxdigit((unsigned char)*(*p)++))
                        return false;
            } else if (ch == 0 || !strchr("\"\\/bfnrt", ch)) {
                return false;
            }
        }
    }
    return false;
}

/* The documented response is flat: strings, integers and a truncated boolean. */
static bool api_status(const char *json, size_t length, int *status) {
    const char *p = json, *end = json + length;
    bool saw_status = false, saw_content = false;
    skip_space(&p, end);
    if (p >= end || *p++ != '{')
        return false;
    for (;;) {
        skip_space(&p, end);
        const char *key, *value;
        size_t key_size, value_size;
        if (!json_string(&p, end, &key, &key_size))
            return false;
        skip_space(&p, end);
        if (p >= end || *p++ != ':')
            return false;
        skip_space(&p, end);
        bool is_status = key_size == 6 && memcmp(key, "status", 6) == 0;
        bool is_content = key_size == 7 && memcmp(key, "content", 7) == 0;
        if (p < end && *p == '"') {
            if (is_status || !json_string(&p, end, &value, &value_size))
                return false;
            if (is_content) {
                if (saw_content)
                    return false;
                saw_content = true;
            }
        } else if (!is_status && !is_content && end - p >= 4 &&
                   (memcmp(p, "true", 4) == 0 || memcmp(p, "null", 4) == 0)) {
            p += 4;
        } else if (!is_status && !is_content && end - p >= 5 &&
                   memcmp(p, "false", 5) == 0) {
            p += 5;
        } else {
            if (is_content || p >= end || (*p != '-' && !isdigit((unsigned char)*p)))
                return false;
            char *number_end;
            errno = 0;
            long long number = strtoll(p, &number_end, 10);
            if (errno != 0 || number_end == p || number_end > end)
                return false;
            p = number_end;
            if (is_status) {
                if (saw_status || number < 0 || number > INT_MAX)
                    return false;
                *status = (int)number;
                saw_status = true;
            }
        }
        skip_space(&p, end);
        if (p >= end)
            return false;
        if (*p == '}') {
            ++p;
            skip_space(&p, end);
            return p == end && saw_status && (*status != 0 || saw_content);
        }
        if (*p++ != ',')
            return false;
    }
}

static bool kernel_snapshot(const char *phase) {
    note("\n[KERNEL %s] source=ShadowMountPlus loopback; limit=%u bytes\n",
         phase, KERNEL_TAIL_BYTES);
    char *data = malloc(HTTP_CAPACITY + 1u);
    if (!data) {
        note("KERNEL_UNAVAILABLE: allocation failed, errno=%d\n", errno);
        return false;
    }
    size_t size = 0;
    if (!kernel_http(data, &size)) {
        int error = errno;
        note("KERNEL_UNAVAILABLE: HTTP transport errno=%d (%s), received=%zu\n",
             error, strerror(error), size);
        free(data);
        return false;
    }
    int http_status = 0, status = -1;
    char *body = strstr(data, "\r\n\r\n");
    bool header_ok = sscanf(data, "HTTP/%*u.%*u %d", &http_status) == 1;
    if (!body || !header_ok || (size_t)(body - data) > 8192u) {
        note("KERNEL_UNAVAILABLE: invalid HTTP response (%zu bytes)\n", size);
        free(data);
        return false;
    }
    body += 4;
    size_t body_size = size - (size_t)(body - data);
    bool json_ok = api_status(body, body_size, &status);
    note("HTTP status=%d; API status=%d; JSON valid=%s; body=%zu bytes\n",
         http_status, status, json_ok ? "yes" : "no", body_size);
    if (fputs("--- RAW KERNEL API JSON BEGIN ---\n", report) == EOF ||
        fwrite(body, 1, body_size, report) != body_size ||
        fputs("\n--- RAW KERNEL API JSON END ---\n", report) == EOF)
        write_failed = true;
    bool ok = http_status == 200 && json_ok && status == 0;
    if (!ok)
        note("KERNEL_UNAVAILABLE: preserve the API error above; no snapshot success claimed.\n");
    free(data);
    return ok;
}

int main(void) {
    report = open_report();
    if (!report) {
        (void)fprintf(stderr, "Launch probe: cannot append %s: errno=%d (%s). No launch attempted.\n",
                      REPORT_PATH, errno, strerror(errno));
        return 2;
    }
    note("\n========== PPSA74207 LAUNCH PROBE 1 ==========%s", "\n");
    note("unix_time=%jd; target=%s; report=%s\n", (intmax_t)time(NULL), TARGET_TITLE, REPORT_PATH);
    note("Only a launch request for the existing registration is made.\n");
    note("API acceptance does not prove _start, rendering, or gameplay. Kernel tails may include other process messages.\n");
    if (!persist_report()) {
        (void)fclose(report);
        return 2;
    }

    bool before = kernel_snapshot("BEFORE");
    int init_status = sceUserServiceInitialize(NULL);
    int32_t user_id = -1;
    /* Initialization may report already initialized; require a valid user read. */
    int user_status = sceUserServiceGetForegroundUser(&user_id);
    note("sceUserServiceInitialize: hex=0x%08" PRIx32 " signed=%" PRId32 "\n",
         (uint32_t)init_status, (int32_t)init_status);
    note("sceUserServiceGetForegroundUser: hex=0x%08" PRIx32 " signed=%" PRId32 "\n",
         (uint32_t)user_status, (int32_t)user_status);
    if (user_status != 0 || user_id < 0) {
        note("NO_LAUNCH: foreground user unavailable; no fallback user selected.\n");
        bool saved = persist_report();
        if (fclose(report) != 0)
            saved = false;
        return saved ? 4 : 2;
    }
    LaunchAppParam parameters = {0};
    parameters.size = sizeof(parameters);
    parameters.user_id = user_id;
    const char *arguments[] = {NULL};
    note("LAUNCH_CALL_BEGIN: %s; monotonic_ms=%" PRId64 "\n", TARGET_TITLE, monotonic_ms());
    if (!persist_report()) {
        (void)fclose(report);
        return 2;
    }
    int result = sceSystemServiceLaunchApp(TARGET_TITLE, arguments, &parameters);
    note("sceSystemServiceLaunchApp(%s): hex=0x%08" PRIx32 " signed=%" PRId32
         " monotonic_ms=%" PRId64 "\n", TARGET_TITLE, (uint32_t)result,
         (int32_t)result, monotonic_ms());
    note("LAUNCH_REQUEST=%s (native return value, not a gameplay test)\n",
         result < 0 ? "REJECTED" : "ACCEPTED");
    if (!persist_report()) {
        (void)fclose(report);
        return 2;
    }
    struct timespec delay = {.tv_sec = 1, .tv_nsec = 0};
    while (nanosleep(&delay, &delay) != 0 && errno == EINTR) {}
    bool after = kernel_snapshot("AFTER");
    note("KERNEL_CAPTURE=%s; before=%s; after=%s\n",
         before && after ? "COMPLETE_TAILS" : "INCOMPLETE",
         before ? "available" : "unavailable", after ? "available" : "unavailable");
    note("END: no install, uninstall, unmount, existing-permission, config, userdata, gamedata, or kstuff changes.\n");
    bool saved = persist_report();
    if (fclose(report) != 0)
        saved = false;
    if (!saved)
        return 2;
    if (result < 0)
        return 1;
    return before && after ? 0 : 3;
}
