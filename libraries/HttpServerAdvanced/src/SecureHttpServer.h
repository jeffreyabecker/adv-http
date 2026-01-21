#pragma once
#include "./StandardHttpServer.h"
#include <WiFiServerSecureBearSSL.h>
#include <type_traits>
#include <utility>
namespace HttpServerAdvanced
{

    namespace Restrictions
    {

        // SFINAE helpers to detect the required members without inheriting from ISecureHttpServerConfig
        template <typename, typename = void>
        struct has_getCache : std::false_type
        {
        };
        template <typename T>
        struct has_getCache<T, std::void_t<decltype(std::declval<T &>().getCache())>>
            : std::is_convertible<decltype(std::declval<T &>().getCache()), BearSSL::ServerSessions *>
        {
        };

        template <typename, typename = void>
        struct has_getCertificateChain : std::false_type
        {
        };
        template <typename T>
        struct has_getCertificateChain<T, std::void_t<decltype(std::declval<T &>().getCertificateChain())>>
            : std::is_convertible<decltype(std::declval<T &>().getCertificateChain()), BearSSL::X509List *>
        {
        };

        template <typename, typename = void>
        struct has_getPrivateKey : std::false_type
        {
        };
        template <typename T>
        struct has_getPrivateKey<T, std::void_t<decltype(std::declval<T &>().getPrivateKey())>>
            : std::is_convertible<decltype(std::declval<T &>().getPrivateKey()), BearSSL::PrivateKey *>
        {
        };

        template <typename, typename = void>
        struct has_getKeyType : std::false_type
        {
        };
        template <typename T>
        struct has_getKeyType<T, std::void_t<decltype(std::declval<T &>().getKeyType())>>
            : std::is_convertible<decltype(std::declval<T &>().getKeyType()), uint32_t>
        {
        };

        // Combine all checks and ensure T does NOT derive from ISecureHttpServerConfig
        template <typename T>
        struct is_secure_http_server_config_like
            : std::conjunction<
                  has_getCache<T>,
                  has_getCertificateChain<T>,
                  has_getPrivateKey<T>,
                  has_getKeyType<T>,
                  std::negation<std::is_base_of<ISecureHttpServerConfig, std::remove_cv_t<std::remove_reference_t<T>>>>>
        {
        };

        template <typename T>
        inline constexpr bool is_secure_http_server_config_like_v = is_secure_http_server_config_like<T>::value;

        // Convenience alias for SFINAE
        template <typename T, typename = std::enable_if_t<is_secure_http_server_config_like_v<T>>>
        using enable_if_secure_http_server_config_t = void;

    } // namespace Restrictions
    template <uint32_t KEY_TYPE, size_t CACHE_SIZE = 8,
              typename = std::enable_if_t<(KEY_TYPE | BR_KEYTYPE_EC > 0) || (KEY_TYPE | BR_KEYTYPE_RSA > 0)>>
    class SecureHttpServerConfig 
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
        


        BearSSL::ServerSessions *getCache() { return &cache_; }
        BearSSL::X509List *getCertificateChain() { return &chain_; }
        BearSSL::PrivateKey *getPrivateKey() { return &sk_; }
        uint32_t getKeyType() { return KEY_TYPE | BR_KEYTYPE_SIGN | BR_KEYTYPE_KEYX; }

        constexpr static uint32_t EC_KEY = BR_KEYTYPE_EC;
        constexpr static uint32_t RSA_KEY = BR_KEYTYPE_RSA;
    };
    // TODO: add a config that loads the certs from SPIFFS or LittleFS

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