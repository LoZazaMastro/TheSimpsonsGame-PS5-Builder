#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

long ps5_pathconf(const char *path, int name);
static int queried_descriptor = -1;
static int indeterminate;

// Exercise the production target branch while forwarding real host queries.
long _fpathconf(int descriptor, int name)
{
    queried_descriptor = descriptor;
    if (indeterminate) {
        return -1;
    }
    return fpathconf(descriptor, name);
}

static void check_closed(void)
{
    assert(queried_descriptor >= 0);
    errno = 0;
    assert(fcntl(queried_descriptor, F_GETFD) == -1 && errno == EBADF);
}

int main(void)
{
    char directory[] = "/tmp/prospero-pathconf-XXXXXX";
    assert(mkdtemp(directory));
    const long expected = pathconf(directory, _PC_NAME_MAX);
    assert(ps5_pathconf(directory, _PC_NAME_MAX) == expected);
    check_closed();

    errno = 0;
    assert(ps5_pathconf(directory, -12345) == -1 && errno == EINVAL);
    check_closed();

    indeterminate = 1;
    errno = ERANGE;
    assert(ps5_pathconf(directory, _PC_NAME_MAX) == -1 && errno == ERANGE);
    check_closed();
    indeterminate = 0;

    char fifo[256];
    assert(snprintf(fifo, sizeof(fifo), "%s/fifo", directory) > 0);
    assert(mkfifo(fifo, 0600) == 0);
    assert(ps5_pathconf(fifo, _PC_PIPE_BUF) == pathconf(fifo, _PC_PIPE_BUF));
    check_closed();
    assert(unlink(fifo) == 0);

    errno = 0;
    assert(ps5_pathconf(fifo, _PC_NAME_MAX) == -1 && errno == ENOENT);
    errno = 0;
    assert(ps5_pathconf(NULL, _PC_NAME_MAX) == -1 && errno == EFAULT);
    assert(rmdir(directory) == 0);
    puts("pathconf host boundary: PASS (native forwarding, errors, indeterminate result, descriptor cleanup, nonblocking FIFO)");
    return 0;
}
