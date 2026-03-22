#pragma once
#include "WebServerBuilder.h"
#include "WebServerConfig.h"
#include <type_traits>
#include <utility>
namespace HttpServerAdvanced
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
            : THttpServer(std::forward<TArgs>(args)...), builder_(*this), config_(*this, builder_)
        {
            static_assert(std::is_convertible_v<THttpServer*, HttpServerBase*>, "THttpServer must be convertible to HttpServerBase");
            builder_.init();
        }
        WebServerConfig& use(std::function<void(WebServerBuilder &)> component){
             config_.use(component);
             return config_;
        }
        WebServerConfig &cfg()
        {
            return config_;
        }
    };

    

}

