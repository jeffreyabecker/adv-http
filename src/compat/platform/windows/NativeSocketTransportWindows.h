#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstddef>
#include <cstdint>

namespace HttpServerAdvanced {
namespace Compat {
namespace platform {

using SocketHandle = SOCKET;
constexpr SocketHandle InvalidSocketHandle = INVALID_SOCKET;

class SocketRuntime {
public:
  SocketRuntime() {
    WSADATA wsaData{};
    initialized_ = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
  }

  ~SocketRuntime() {
    if (initialized_) {
      WSACleanup();
    }
  }

  bool initialized() const { return initialized_; }

private:
  bool initialized_ = false;
};

inline int lastSocketError() { return WSAGetLastError(); }

inline bool isWouldBlockError(int errorCode) {
  return errorCode == WSAEWOULDBLOCK || errorCode == WSAETIMEDOUT;
}

inline bool isInterruptedError(int errorCode) { return errorCode == WSAEINTR; }

inline int closeSocket(SocketHandle handle) { return closesocket(handle); }

inline bool setNonBlocking(SocketHandle handle, bool enabled) {
  u_long mode = enabled ? 1UL : 0UL;
  return ioctlsocket(handle, FIONBIO, &mode) == 0;
}

inline bool setSocketTimeout(SocketHandle handle, std::uint32_t timeoutMs) {
  const DWORD timeout = static_cast<DWORD>(timeoutMs);
  return setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<const char *>(&timeout),
                    sizeof(timeout)) == 0 &&
         setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO,
                    reinterpret_cast<const char *>(&timeout),
                    sizeof(timeout)) == 0;
}

inline bool queryPendingByteCount(SocketHandle handle,
                                  std::size_t &pendingBytes) {
  u_long nativePendingBytes = 0;
  if (ioctlsocket(handle, FIONREAD, &nativePendingBytes) != 0) {
    return false;
  }

  pendingBytes = static_cast<std::size_t>(nativePendingBytes);
  return true;
}

} // namespace platform
} // namespace Compat
} // namespace HttpServerAdvanced