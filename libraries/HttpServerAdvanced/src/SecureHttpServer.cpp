#include "./SecureHttpServer.h"
#include <cassert>

namespace HttpServerAdvanced
{
    SecureHttpServer::SecureHttpServer(uint16_t port, const IPAddress &ip)
        : HttpServer<BearSSL::WiFiServerSecure, 443>(port, ip)
    {
    }

    SecureHttpServer::~SecureHttpServer() = default;

    void SecureHttpServer::begin()
    {
        assert(false && "SecureHttpServer::begin() must be called passing an ISecureHttpServerConfig instance");
    }

    void SecureHttpServer::begin(ISecureHttpServerConfig &config)
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
}