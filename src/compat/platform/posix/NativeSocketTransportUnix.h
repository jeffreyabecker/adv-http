#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>

namespace HttpServerAdvanced {
namespace Compat {
namespace platform {

using SocketHandle = int;
constexpr SocketHandle InvalidSocketHandle = -1;

class SocketRuntime {
public:
  bool initialized() const { return true; }
};

inline int lastSocketError() { return errno; }

inline bool isWouldBlockError(int errorCode) {
  return errorCode == EAGAIN || errorCode == EWOULDBLOCK ||
         errorCode == ETIMEDOUT;
}

inline bool isInterruptedError(int errorCode) { return errorCode == EINTR; }

inline int closeSocket(SocketHandle handle) { return close(handle); }

inline bool setNonBlocking(SocketHandle handle, bool enabled) {
  const int flags = fcntl(handle, F_GETFL, 0);
  if (flags < 0) {
    return false;
  }

  const int updatedFlags =
      enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
  return fcntl(handle, F_SETFL, updatedFlags) == 0;
}

inline bool setSocketTimeout(SocketHandle handle, std::uint32_t timeoutMs) {
  timeval timeout{};
  timeout.tv_sec = static_cast<long>(timeoutMs / 1000U);
  timeout.tv_usec = static_cast<long>((timeoutMs % 1000U) * 1000U);
  return setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                    sizeof(timeout)) == 0 &&
         setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                    sizeof(timeout)) == 0;
}

inline bool queryPendingByteCount(SocketHandle handle,
                                  std::size_t &pendingBytes) {
  int nativePendingBytes = 0;
  if (ioctl(handle, FIONREAD, &nativePendingBytes) != 0) {
    return false;
  }

  pendingBytes =
      nativePendingBytes > 0 ? static_cast<std::size_t>(nativePendingBytes) : 0;
  return true;
}

} // namespace platform
} // namespace Compat
} // namespace HttpServerAdvanced