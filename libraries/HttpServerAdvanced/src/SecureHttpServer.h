#pragma once
#include "./StandardHttpServer.h"
#include "./SecureHttpServerConfig.h"
#include "./pipeline/NetClient.h"
#include <WiFi.h>
#include <type_traits>
#include <utility>

namespace HttpServerAdvanced
{

    class SecureHttpServer : public StandardHttpServer<BearSSL::WiFiServerSecure, 443>
    {
    private:
        ISecureHttpServerConfig *config_;

    public:
        SecureHttpServer(uint16_t port = 443, const IPAddress &ip = IPAddress(IPADDR_ANY)) : StandardHttpServer<BearSSL::WiFiServerSecure, 443>(port, ip), config_(nullptr)
        {
        }
        SecureHttpServer(ISecureHttpServerConfig &config, uint16_t port = 443, const IPAddress &ip = IPAddress(IPADDR_ANY))
            : StandardHttpServer<BearSSL::WiFiServerSecure, 443>(port, ip), config_(&config)
        {
        }
        ~SecureHttpServer() override;

        void begin() override
        {
            return begin(*config_);
        }

        void begin(ISecureHttpServerConfig &config)
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

            StandardHttpServer<BearSSL::WiFiServerSecure, 443>::begin();
        }
    };
}

