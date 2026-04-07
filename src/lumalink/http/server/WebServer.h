#pragma once
#include "WebServerBuilder.h"
#include "WebServerConfig.h"
#include <type_traits>
#include <utility>

namespace lumalink::http::server
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
  using WebServer = FriendlyWebServer<HttpServerBase>;

} // namespace lumalink::http::server
