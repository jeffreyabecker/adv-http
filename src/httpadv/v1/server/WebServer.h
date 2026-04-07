#pragma once
#include "WebServerBuilder.h"
#include "WebServerConfig.h"
#include <type_traits>
#include <utility>
#include "lumalink/platform/transport/TransportTraits.h"
#ifdef ARDUINO
#include "lumalink/platform/arduino/ArduinoWiFiTransport.h"
#elif defined(_WIN32)
#include "lumalink/platform/windows/WindowsSocketTransport.h"
#else
#include "lumalink/platform/posix/PosixSocketTransport.h"
#endif
namespace httpadv::v1::server
{
  template <typename THttpServer>
  class FriendlyWebServer : public THttpServer
  {
  private:
    WebServerBuilder builder_;
    WebServerConfig config_;

  public:
    template <typename... TArgs>
    explicit FriendlyWebServer(TArgs &&...args)
        : THttpServer(std::forward<TArgs>(args)...), builder_(*this),
          config_(*this, builder_)
    {
      static_assert(std::is_convertible_v<THttpServer *, HttpServerBase *>,
                    "THttpServer must be convertible to HttpServerBase");
      builder_.init();
    }
    template <typename TComponent>
    WebServerConfig &use(TComponent &&component)
    {
      static_assert(std::is_invocable_v<TComponent, WebServerBuilder &>,
                    "component must be invocable with WebServerBuilder&");
      config_.use(std::forward<TComponent>(component));
      return config_;
    }
    WebServerConfig &cfg() { return config_; }
  };
  namespace detail
  {
#ifdef ARDUINO
    using NativeTransportFactory = lumalink::platform::arduino::ArduinoWiFiTransportFactory;
#elif defined(_WIN32)
    using NativeTransportFactory = lumalink::platform::windows::WindowsSocketTransportFactory;
#else
    using NativeTransportFactory = lumalink::platform::posix::PosixSocketTransportFactory;
#endif

  }
  template <typename TransportFactory = detail::NativeTransportFactory,
        typename = std::enable_if_t<lumalink::platform::transport::IsStaticTransportFactoryV<TransportFactory>>>
  class PlatformWebServer : public FriendlyWebServer<HttpServerBase>
  {
  public:
    PlatformWebServer(uint16_t port = 80) : FriendlyWebServer<HttpServerBase>(TransportFactory::createServer(port)) {}
  };

  using WebServer = FriendlyWebServer<HttpServerBase>;

} // namespace httpadv::v1::server
