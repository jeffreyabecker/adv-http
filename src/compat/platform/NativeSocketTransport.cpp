#include "NativeSocketTransport.h"

#if !defined(ARDUINO)

#if defined(_WIN32)
#include "windows/WindowsSocketTransport.h"
#else
#include "posix/PosixSocketTransport.h"
#endif

namespace HttpServerAdvanced {

std::unique_ptr<ITransportFactory> createNativeSocketTransportFactory() {
#if defined(_WIN32)
  return std::make_unique<Compat::platform::windows::NativeSocketTransportFactory>();
#else
  return std::make_unique<Compat::platform::posix::NativeSocketTransportFactory>();
#endif
}
} // namespace HttpServerAdvanced

#else

namespace HttpServerAdvanced {
std::unique_ptr<ITransportFactory> createNativeSocketTransportFactory() {
  return nullptr;
}
} // namespace HttpServerAdvanced

#endif