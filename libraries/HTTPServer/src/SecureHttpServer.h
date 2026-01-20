#pragma once
#include "./StandardHttpServer.h"
#include <WiFiServerSecureBearSSL.h>

namespace HTTPServer
{
    class ISecureHttpServerConfig
    {
    public:
        virtual ~ISecureHttpServerConfig() = default;
        virtual BearSSL::ServerSessions *getCache() = 0;
        virtual BearSSL::X509List *getCertificateChain() = 0;
        virtual BearSSL::PrivateKey *getPrivateKey() = 0;
        virtual uint32_t getKeyType() { return 0; }
    };

    template<uint32_t KEY_TYPE, size_t CACHE_SIZE = 8,
             typename = std::enable_if_t<(KEY_TYPE | BR_KEYTYPE_EC > 0) || (KEY_TYPE | BR_KEYTYPE_RSA > 0)>>
    class SecureHttpServerConfig : public ISecureHttpServerConfig
    {
    protected:
        BearSSL::X509List chain_;
        BearSSL::PrivateKey sk_;
        BearSSL::ServerSessions cache_;
        uint32_t keyType_;

    public:
        SecureHttpServerConfig(const char *serverCert, const char *privateKey)
            : chain_(serverCert), sk_(privateKey), cache_(CACHE_SIZE), keyType_(KEY_TYPE)
        {
        }

        ~SecureHttpServerConfig() override = default;

        BearSSL::ServerSessions *getCache() override { return &cache_; }
        BearSSL::X509List *getCertificateChain() override { return &chain_; }
        BearSSL::PrivateKey *getPrivateKey() override { return &sk_; }
        uint32_t getKeyType() override { return KEY_TYPE | BR_KEYTYPE_SIGN | BR_KEYTYPE_KEYX; }

        constexpr static uint32_t EC_KEY = BR_KEYTYPE_EC;
        constexpr static uint32_t RSA_KEY = BR_KEYTYPE_RSA;
    };
    //TODO: add a config that loads the certs from SPIFFS or LittleFS

    class SecureHttpServer : public HttpServer<BearSSL::WiFiServerSecure, 443>
    {
    public:
        SecureHttpServer(uint16_t port = 443, const IPAddress &ip = IPAddress(IPADDR_ANY));
        ~SecureHttpServer() override;

        void begin() override;
        void begin(ISecureHttpServerConfig &config);
    };
}