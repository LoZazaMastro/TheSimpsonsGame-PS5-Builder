#include <rex/net/socket.h>
#include <rex/platform.h>

static_assert(REX_PLATFORM_LINUX || REX_PLATFORM_MAC || REX_PLATFORM_PROSPERO, "This file is POSIX-only");

#include <sys/ioctl.h>
#include <unistd.h>
#if REX_PLATFORM_PROSPERO
#include <cerrno>
#include <climits>
#include <sys/filio.h>
#include <sys/sockio.h>
#endif

namespace rex::net {

int socket_close(SocketHandle handle) {
  return close(static_cast<int>(handle));
}

int socket_ioctl(SocketHandle handle, uint32_t cmd, uint8_t* arg) {
#if REX_PLATFORM_PROSPERO
  // This boundary accepts native commands/data. XSocket translates Xbox
  // command numbers and the guest's big-endian 32-bit argument before here.
  if (handle < 0 || handle > INT_MAX) {
    errno = EBADF;
    return -1;
  }
  if (!arg) {
    errno = EFAULT;
    return -1;
  }
  if (cmd != FIONBIO && cmd != FIONREAD && cmd != SIOCATMARK) {
    errno = EOPNOTSUPP;
    return -1;
  }
#endif
  return ioctl(static_cast<int>(handle), cmd, arg);
}

}  // namespace rex::net
