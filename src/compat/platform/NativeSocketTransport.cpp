#include "NativeSocketTransport.h"

#if !defined(ARDUINO)

#include "NativeSocketTransportImpl.h"

namespace HttpServerAdvanced {

std::unique_ptr<ITransportFactory> createNativeSocketTransportFactory() {
  return std::make_unique<Compat::platform::NativeSocketTransportFactory>();
}
} // namespace HttpServerAdvanced

#else

namespace HttpServerAdvanced {
std::unique_ptr<ITransportFactory> createNativeSocketTransportFactory() {
  return nullptr;
}
} // namespace HttpServerAdvanced

#endif