#pragma once
#include "./StandardHttpServer.h"
#include "./SecureHttpServerConfig.h"
#include "./NetClient.h"
#include <WiFi.h>
#include <type_traits>
#include <utility>

namespace HttpServerAdvanced
{

    class SecureHttpServer : public HttpServer<BearSSL::WiFiServerSecure, 443>
    {
    public:
        SecureHttpServer(uint16_t port = 443, const IPAddress &ip = IPAddress(IPADDR_ANY)) : HttpServer<BearSSL::WiFiServerSecure, 443>(port, ip)
        {
        }
        ~SecureHttpServer() override;

        void begin() override
        {
            assert(false && "SecureHttpServer::begin() must be called passing an ISecureHttpServerConfig instance");
        }
        template <typename T,
                  typename = Restrictions::enable_if_secure_http_server_config_t<T>>
        void begin(T &config)
        {
            // configure the server before beginning
            configureConnection([&config](BearSSL::WiFiServerSecure *server)
                                {
            auto keyType = config.getKeyType();
            auto cache = config.getCache();
            if (cache)
            {
                server->setCache(cache);
            }
            if ((keyType & BR_KEYTYPE_RSA) != 0)
            {
                server->setRSACert(config.getCertificateChain(), config.getPrivateKey());
            }
            else if ((keyType & BR_KEYTYPE_EC) != 0)
            {
                server->setECCert(config.getCertificateChain(), keyType, config.getPrivateKey());
            }
            else
            {
                assert(false && "SecureHttpServer: Unsupported key type");
            } });

            HttpServer<BearSSL::WiFiServerSecure, 443>::begin();
        }
    };
}