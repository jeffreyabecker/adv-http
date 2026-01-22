#pragma once
#include "./StandardHttpServer.h"
#include "./CoreServices.h"
#include "./WebServerConfig.h"
#include <type_traits>
namespace HttpServerAdvanced
{



    template <typename THttpServer = StandardHttpServer<>, typename... Args>
    class FriendlyWebServer : public THttpServer
    {
    private:
        class WebServerBuilder : public CoreServicesBuilder
        {
        private:
            HttpServerBase &server_;

        public:
            WebServerBuilder(HttpServerBase &server) : CoreServicesBuilder([](CoreServicesBuilder &coreServices) {}),
                                                               server_(server)
            {
            }
            void init()
            {
                CoreServicesBuilder::init(server_);
            }
        };
        WebServerBuilder builder_;
        WebServerConfig config_;

    public:
        FriendlyWebServer(Args... args) : THttpServer(args...), builder_(*this), config_(*this, builder_)
        {
            static_assert(std::is_convertible_v<THttpServer*, HttpServerBase*>, "THttpServer must be convertible to HttpServerBase");
            builder_.init();
        }
        WebServerConfig &cfg()
        {
            return config_;
        }
    };

    

}