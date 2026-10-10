/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <cstring>
#include <cerrno>
#include <cstddef>
#include <limits>

#include <rex/kernel/xam/module.h>
#include <rex/platform.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xsocket.h>
// #include <rex/system/xnet.h>

#include <rex/net/socket.h>

// Standard socket types used by Xbox API emulation
#if REX_PLATFORM_WIN32
#include <winsock2.h>

#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#if REX_PLATFORM_PROSPERO
#include <netinet/tcp.h>
#include <sys/filio.h>
#include <sys/sockio.h>
#include <sys/time.h>
#endif
#endif

namespace rex::system {

#if REX_PLATFORM_PROSPERO
namespace {

static_assert(sizeof(sockaddr_in) == 16 && sizeof(sockaddr) == 16);
static_assert(sizeof(sockaddr_in::sin_family) == 1);
static_assert(offsetof(sockaddr_in, sin_len) == 0 && offsetof(sockaddr_in, sin_family) == 1);
static_assert(offsetof(sockaddr_in, sin_port) == 2 && offsetof(sockaddr_in, sin_addr) == 4);
static_assert(MSG_WAITALL == 0x40 && MSG_EOR == 0x8 && AF_INET == 2);

// N_XSOCKADDR retains a 16-bit family. BSD uses an 8-bit length followed by
// an 8-bit family; only the six network-order port/address bytes can be copied.
template <typename XboxAddress>
bool NativeAddress(const XboxAddress* source, int length, uint16_t family, sockaddr_in& target) {
  if (!source || length < int(sizeof(XboxAddress))) {
    errno = EFAULT;
    return false;
  }
  if (family != AF_INET) {
    errno = EAFNOSUPPORT;
    return false;
  }
  target = {};
  target.sin_len = sizeof(target);
  target.sin_family = AF_INET;
  std::memcpy(&target.sin_port, reinterpret_cast<const uint8_t*>(source) + 2, 2);
  std::memcpy(&target.sin_addr.s_addr, reinterpret_cast<const uint8_t*>(source) + 4, 4);
  return true;
}

bool MessageFlags(uint32_t xbox, bool sending, int& native) {
  constexpr uint32_t kOob = 1, kPeek = 2, kDontRoute = 4, kWaitAll = 8;
  const uint32_t allowed = sending ? kOob | kDontRoute : kOob | kPeek | kWaitAll;
  if (xbox & ~allowed) {
    errno = EOPNOTSUPP;
    return false;
  }
  native = sending ? MSG_NOSIGNAL : 0;
  if (xbox & kOob) native |= MSG_OOB;
  if (xbox & kPeek) native |= MSG_PEEK;
  if (xbox & kDontRoute) native |= MSG_DONTROUTE;
  // Xbox/Winsock's 0x8 means WAITALL; BSD's raw 0x8 means EOR.
  if (xbox & kWaitAll) native |= MSG_WAITALL;
  return true;
}

bool BufferLength(const void* data, uint32_t length) {
  if (!data && length) {
    errno = EFAULT;
    return false;
  }
  if (length > uint32_t(std::numeric_limits<int>::max())) {
    errno = EMSGSIZE;
    return false;
  }
  return true;
}

uint32_t GuestU32(const void* data) {
  const auto* bytes = static_cast<const uint8_t*>(data);
  return (uint32_t(bytes[0]) << 24) | (uint32_t(bytes[1]) << 16) |
         (uint32_t(bytes[2]) << 8) | bytes[3];
}

int SetNativeOption(int fd, uint32_t level, uint32_t option, const void* value, uint32_t length) {
  if (!value) {
    errno = EFAULT;
    return -1;
  }
  int native_level = 0, native_option = 0;
  bool timeout = false, linger_option = false, dont_linger = false;
  if (level == 0xffff) {
    native_level = SOL_SOCKET;
    switch (option) {
      case 0x0004: native_option = SO_REUSEADDR; break;
      case 0x0008: native_option = SO_KEEPALIVE; break;
      case 0x0010: native_option = SO_DONTROUTE; break;
      case 0x0020: native_option = SO_BROADCAST; break;
      case 0x0080: native_option = SO_LINGER; linger_option = true; break;
      case 0x0100: native_option = SO_OOBINLINE; break;
      case 0x1001: native_option = SO_SNDBUF; break;
      case 0x1002: native_option = SO_RCVBUF; break;
      case 0x1003: native_option = SO_SNDLOWAT; break;
      case 0x1004: native_option = SO_RCVLOWAT; break;
      case 0x1005: native_option = SO_SNDTIMEO; timeout = true; break;
      case 0x1006: native_option = SO_RCVTIMEO; timeout = true; break;
      case 0xffffff7f: native_option = SO_LINGER; dont_linger = true; break;
      // Xbox encryption/XNet options are not native socket options.
      default: errno = ENOPROTOOPT; return -1;
    }
  } else if (level == 6 && option == 1) {
    native_level = IPPROTO_TCP;
    native_option = TCP_NODELAY;
  } else if (level == 0) {
    native_level = IPPROTO_IP;
    switch (option) {
      case 3: native_option = IP_TOS; break;
      case 4: native_option = IP_TTL; break;
      case 14: native_option = IP_DONTFRAG; break;
      default: errno = ENOPROTOOPT; return -1;
    }
  } else {
    errno = ENOPROTOOPT;
    return -1;
  }
  if (length < 4) {
    errno = EFAULT;
    return -1;
  }
  const uint32_t word = GuestU32(value);
  if (linger_option || dont_linger) {
    // Winsock's linger is two 16-bit integers; BSD's is two native ints.
    linger native{};
    if (dont_linger) {
      socklen_t size = sizeof(native);
      if (getsockopt(fd, SOL_SOCKET, SO_LINGER, &native, &size) < 0) return -1;
      if (size != sizeof(native)) {
        errno = EINVAL;
        return -1;
      }
      native.l_onoff = !word;
    } else {
      native.l_onoff = (word >> 16) != 0;
      native.l_linger = word & 0xffff;
    }
    return setsockopt(fd, native_level, native_option, &native, sizeof(native));
  }
  if (timeout) {
    // Winsock uses a DWORD in milliseconds; BSD requires a native timeval.
    timeval native{};
    native.tv_sec = word / 1000;
    native.tv_usec = (word % 1000) * 1000;
    return setsockopt(fd, native_level, native_option, &native, sizeof(native));
  }
  int native = static_cast<int32_t>(word);
  return setsockopt(fd, native_level, native_option, &native, sizeof(native));
}

}  // namespace

uint32_t XSocket::GetLastWSAError() {
  switch (errno) {
    case EINTR: return 10004;
    case EACCES: case EPERM: return 10013;
    case EFAULT: return 10014;
    case EINVAL: return 10022;
    case EMFILE: case ENFILE: return 10024;
    case EAGAIN: return 10035;
    case EINPROGRESS: return 10036;
    case EALREADY: return 10037;
    case EBADF: case ENOTSOCK: return 10038;
    case EDESTADDRREQ: return 10039;
    case EMSGSIZE: return 10040;
    case EPROTOTYPE: return 10041;
    case ENOPROTOOPT: return 10042;
    case EPROTONOSUPPORT: return 10043;
    case ESOCKTNOSUPPORT: return 10044;
    case EOPNOTSUPP: case ENOSYS: return 10045;
    case EPFNOSUPPORT: return 10046;
    case EAFNOSUPPORT: return 10047;
    case EADDRINUSE: return 10048;
    case EADDRNOTAVAIL: return 10049;
    case ENETDOWN: return 10050;
    case ENETUNREACH: return 10051;
    case ENETRESET: return 10052;
    case ECONNABORTED: return 10053;
    case ECONNRESET: return 10054;
    case ENOBUFS: case ENOMEM: return 10055;
    case EISCONN: return 10056;
    case ENOTCONN: return 10057;
    case ESHUTDOWN: case EPIPE: return 10058;
    case ETIMEDOUT: return 10060;
    case ECONNREFUSED: return 10061;
    case EHOSTDOWN: return 10064;
    case EHOSTUNREACH: return 10065;
    default: return 10107;  // WSASYSCALLFAILURE, never fabricate a zero error.
  }
}
#endif

XSocket::XSocket(KernelState* kernel_state) : XObject(kernel_state, kObjectType) {}

XSocket::XSocket(KernelState* kernel_state, uint64_t native_handle)
    : XObject(kernel_state, kObjectType), native_handle_(native_handle) {}

XSocket::~XSocket() {
  Close();
}

X_STATUS XSocket::Initialize(AddressFamily af, Type type, Protocol proto) {
#if REX_PLATFORM_PROSPERO
  if (af != 2) {
    errno = EAFNOSUPPORT;
    return X_STATUS_UNSUCCESSFUL;
  }
  if (type != 1 && type != 2) {
    errno = ESOCKTNOSUPPORT;
    return X_STATUS_UNSUCCESSFUL;
  }
  if (proto != 0 && proto != (type == 1 ? 6 : 17)) {
    // VDP needs XNet security/framing, which this backend does not implement.
    errno = EPROTONOSUPPORT;
    return X_STATUS_UNSUCCESSFUL;
  }
#endif
  af_ = af;
  type_ = type;
  proto_ = proto;

  if (proto == Protocol::IPPROTO_VDP) {
    // VDP is a layer on top of UDP.
    proto = static_cast<Protocol>(IPPROTO_UDP);
  }

  native_handle_ = socket(af, type, proto);
  if (native_handle_ == uint64_t(-1)) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Close() {
#if REX_PLATFORM_PROSPERO
  if (native_handle_ == uint64_t(-1)) return X_STATUS_SUCCESS;
  const auto handle = native_handle_;
  native_handle_ = uint64_t(-1);  // Destructor must not close a reused descriptor.
  int ret = rex::net::socket_close(handle);
#else
  int ret = rex::net::socket_close(native_handle_);
#endif
  if (ret != 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::SetOption(uint32_t level, uint32_t optname, void* optval_ptr, uint32_t optlen) {
#if REX_PLATFORM_PROSPERO
  if (SetNativeOption(static_cast<int>(native_handle_), level, optname, optval_ptr, optlen) < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }
  if (level == 0xffff && optname == 0x0020) broadcast_socket_ = GuestU32(optval_ptr) != 0;
  return X_STATUS_SUCCESS;
#else
  if (level == 0xFFFF && (optname == 0x5801 || optname == 0x5802)) {
    // Disable socket encryption
    secure_ = false;
    return X_STATUS_SUCCESS;
  }

  int ret = setsockopt(native_handle_, level, optname, (char*)optval_ptr, optlen);
  if (ret < 0) {
    // TODO: WSAGetLastError()
    return X_STATUS_UNSUCCESSFUL;
  }

  // SO_BROADCAST
  if (level == 0xFFFF && optname == 0x0020) {
    broadcast_socket_ = true;
  }

  return X_STATUS_SUCCESS;
#endif
}

X_STATUS XSocket::IOControl(uint32_t cmd, uint8_t* arg_ptr) {
#if REX_PLATFORM_PROSPERO
  if (!arg_ptr) {
    errno = EFAULT;
    return X_STATUS_UNSUCCESSFUL;
  }
  uint32_t native_command;
  switch (cmd) {
    case 0x8004667e: native_command = FIONBIO; break;
    case 0x4004667f: native_command = FIONREAD; break;
    case 0x40047307: native_command = SIOCATMARK; break;
    default: errno = EOPNOTSUPP; return X_STATUS_UNSUCCESSFUL;
  }
  int value = cmd == 0x8004667e ? GuestU32(arg_ptr) != 0 : 0;
  const int ret = rex::net::socket_ioctl(native_handle_, native_command,
                                         reinterpret_cast<uint8_t*>(&value));
  if (ret < 0) return X_STATUS_UNSUCCESSFUL;
  if (cmd != 0x8004667e) {
    const uint32_t word = static_cast<uint32_t>(value);
    arg_ptr[0] = word >> 24;
    arg_ptr[1] = word >> 16;
    arg_ptr[2] = word >> 8;
    arg_ptr[3] = word;
  }
  return X_STATUS_SUCCESS;
#else
  int ret = rex::net::socket_ioctl(native_handle_, cmd, arg_ptr);
  if (ret < 0) {
    // TODO: Get last error
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
#endif
}

X_STATUS XSocket::Connect(N_XSOCKADDR* name, int name_len) {
#if REX_PLATFORM_PROSPERO
  sockaddr_in native{};
  if (!NativeAddress(name, name_len, name ? name->address_family : 0, native)) {
    return X_STATUS_UNSUCCESSFUL;
  }
  int ret = connect(native_handle_, reinterpret_cast<sockaddr*>(&native), sizeof(native));
#else
  int ret = connect(native_handle_, (sockaddr*)name, name_len);
#endif
  if (ret < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Bind(N_XSOCKADDR_IN* name, int name_len) {
#if REX_PLATFORM_PROSPERO
  sockaddr_in native{};
  if (!NativeAddress(name, name_len, name ? name->sin_family : 0, native)) {
    return X_STATUS_UNSUCCESSFUL;
  }
  int ret = bind(native_handle_, reinterpret_cast<sockaddr*>(&native), sizeof(native));
#else
  int ret = bind(native_handle_, (sockaddr*)name, name_len);
#endif
  if (ret < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  bound_ = true;
  bound_port_ = name->sin_port;

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Listen(int backlog) {
  int ret = listen(native_handle_, backlog);
  if (ret < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

object_ref<XSocket> XSocket::Accept(N_XSOCKADDR* name, int* name_len) {
#if REX_PLATFORM_PROSPERO
  if (name && (!name_len || *name_len < int(sizeof(*name)))) {
    errno = EFAULT;
    return nullptr;
  }
  sockaddr_in native{};
  socklen_t native_length = sizeof(native);
  const int ret = accept(native_handle_, name ? reinterpret_cast<sockaddr*>(&native) : nullptr,
                          name ? &native_length : nullptr);
  if (ret < 0) return nullptr;
  if (name) {
    if (native_length < sizeof(native) || native.sin_family != AF_INET) {
      rex::net::socket_close(ret);
      errno = EAFNOSUPPORT;
      return nullptr;
    }
    *name = N_XSOCKADDR{};
    name->address_family = AF_INET;
    std::memcpy(name->sa_data, &native.sin_port, 2);
    std::memcpy(name->sa_data + 2, &native.sin_addr.s_addr, 4);
    *name_len = sizeof(*name);
  }
#else
  sockaddr n_sockaddr;
  socklen_t n_name_len = sizeof(sockaddr);
  uintptr_t ret = accept(native_handle_, &n_sockaddr, &n_name_len);
  if (ret == -1) {
    std::memset(name, 0, *name_len);
    *name_len = 0;
    return nullptr;
  }

  std::memcpy(name, &n_sockaddr, n_name_len);
  *name_len = n_name_len;
#endif

  // Create a kernel object to represent the new socket, and copy parameters
  // over.
  auto socket = object_ref<XSocket>(new XSocket(kernel_state_, ret));
  socket->af_ = af_;
  socket->type_ = type_;
  socket->proto_ = proto_;

  return socket;
}

int XSocket::Shutdown(int how) {
#if REX_PLATFORM_PROSPERO
  if (how < 0 || how > 2) {
    errno = EINVAL;
    return -1;
  }
#endif
  return shutdown(native_handle_, how);
}

int XSocket::Recv(uint8_t* buf, uint32_t buf_len, uint32_t flags) {
#if REX_PLATFORM_PROSPERO
  int native_flags;
  if (!BufferLength(buf, buf_len) || !MessageFlags(flags, false, native_flags)) return -1;
  return recv(native_handle_, buf, buf_len, native_flags);
#else
  return recv(native_handle_, reinterpret_cast<char*>(buf), buf_len, flags);
#endif
}

int XSocket::RecvFrom(uint8_t* buf, uint32_t buf_len, uint32_t flags, N_XSOCKADDR_IN* from,
                      uint32_t* from_len) {
#if REX_PLATFORM_PROSPERO
  int native_flags;
  if (!BufferLength(buf, buf_len) || !MessageFlags(flags, false, native_flags)) return -1;
  if (from && (!from_len || *from_len < sizeof(*from))) {
    errno = EFAULT;
    return -1;
  }
  sockaddr_in native{};
  socklen_t native_length = sizeof(native);
  const int ret = recvfrom(native_handle_, buf, buf_len, native_flags,
                            from ? reinterpret_cast<sockaddr*>(&native) : nullptr,
                            from ? &native_length : nullptr);
  if (ret >= 0 && from) {
    if (native_length < sizeof(native) || native.sin_family != AF_INET) {
      errno = EAFNOSUPPORT;
      return -1;
    }
    from->sin_family = AF_INET;
    from->sin_port = ntohs(native.sin_port);
    from->sin_addr = ntohl(native.sin_addr.s_addr);
    std::memset(from->x_sin_zero, 0, sizeof(from->x_sin_zero));
    *from_len = sizeof(*from);
  }
  return ret;
#else
  // Pop from secure packets first
  // TODO(DrChat): Enable when I commit XNet
  /*
  {
    std::lock_guard<std::mutex> lock(incoming_packet_mutex_);
    if (incoming_packets_.size()) {
      packet* pkt = (packet*)incoming_packets_.front();
      int data_len = pkt->data_len;
      std::memcpy(buf, pkt->data, std::min((uint32_t)pkt->data_len, buf_len));

      from->sin_family = 2;
      from->sin_addr = pkt->src_ip;
      from->sin_port = pkt->src_port;

      incoming_packets_.pop();
      uint8_t* pkt_ui8 = (uint8_t*)pkt;
      delete[] pkt_ui8;

      return data_len;
    }
  }
  */

  sockaddr_in nfrom;
  socklen_t nfromlen = sizeof(sockaddr_in);
  int ret = recvfrom(native_handle_, reinterpret_cast<char*>(buf), buf_len, flags,
                     (sockaddr*)&nfrom, &nfromlen);
  if (from) {
    from->sin_family = nfrom.sin_family;
    from->sin_addr = ntohl(nfrom.sin_addr.s_addr);  // BE <- BE
    from->sin_port = nfrom.sin_port;
    std::memset(from->x_sin_zero, 0, sizeof(from->x_sin_zero));
  }

  if (from_len) {
    *from_len = nfromlen;
  }

  return ret;
#endif
}

int XSocket::Send(const uint8_t* buf, uint32_t buf_len, uint32_t flags) {
#if REX_PLATFORM_PROSPERO
  int native_flags;
  if (!BufferLength(buf, buf_len) || !MessageFlags(flags, true, native_flags)) return -1;
  return send(native_handle_, buf, buf_len, native_flags);
#else
  return send(native_handle_, reinterpret_cast<const char*>(buf), buf_len, flags);
#endif
}

int XSocket::SendTo(uint8_t* buf, uint32_t buf_len, uint32_t flags, N_XSOCKADDR_IN* to,
                    uint32_t to_len) {
#if REX_PLATFORM_PROSPERO
  int native_flags;
  if (!BufferLength(buf, buf_len) || !MessageFlags(flags, true, native_flags)) return -1;
  sockaddr_in native{};
  if (to && !NativeAddress(to, static_cast<int>(to_len), to->sin_family, native)) return -1;
  if (!to && to_len) {
    errno = EFAULT;
    return -1;
  }
  return sendto(native_handle_, buf, buf_len, native_flags,
                  to ? reinterpret_cast<sockaddr*>(&native) : nullptr, to ? sizeof(native) : 0);
#else
  // Send 2 copies of the packet: One to XNet (for network security) and an
  // unencrypted copy for other Xenia hosts.
  // TODO(DrChat): Enable when I commit XNet.
  /*
  auto xam = kernel_state()->GetKernelModule<xam::XamModule>("xam.xex");
  auto xnet = xam->xnet();
  if (xnet) {
    xnet->SendPacket(this, to, buf, buf_len);
  }
  */

  sockaddr_in nto;
  if (to) {
    nto.sin_addr.s_addr = to->sin_addr;
    nto.sin_family = to->sin_family;
    nto.sin_port = to->sin_port;
  }

  return sendto(native_handle_, reinterpret_cast<char*>(buf), buf_len, flags,
                to ? (sockaddr*)&nto : nullptr, to_len);
#endif
}

bool XSocket::QueuePacket(uint32_t src_ip, uint16_t src_port, const uint8_t* buf, size_t len) {
  packet* pkt = reinterpret_cast<packet*>(new uint8_t[sizeof(packet) + len]);
  pkt->src_ip = src_ip;
  pkt->src_port = src_port;

  pkt->data_len = (uint16_t)len;
  std::memcpy(pkt->data, buf, len);

  std::lock_guard<std::mutex> lock(incoming_packet_mutex_);
  incoming_packets_.push((uint8_t*)pkt);

  // TODO: Limit on number of incoming packets?
  return true;
}

}  // namespace rex::system
