// Host-only syscall-boundary injection. LD_PRELOAD reaches lstat calls made
// inside libstdc++, unlike --wrap=lstat applied only to the test executable.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static unsigned long denied_calls;

unsigned long profile_lstat_eperm_calls(void) { return denied_calls; }

int lstat(const char* path, struct stat* info) {
  const char* root = getenv("PROSPERO_PROFILE_DENY_LSTAT_ROOT");
  if (root && *root) {
    const size_t length = strlen(root);
    if (strncmp(path, root, length) == 0 &&
        (path[length] == '/' || path[length] == '\0')) {
      ++denied_calls;
      errno = EPERM;
      return -1;
    }
  }
  typedef int (*lstat_fn)(const char*, struct stat*);
  const lstat_fn actual = (lstat_fn)dlsym(RTLD_NEXT, "lstat");
  if (!actual) {
    errno = ENOSYS;
    return -1;
  }
  return actual(path, info);
}
