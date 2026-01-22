#pragma once
#include "./Defines.h"
#include "./StandardHttpServer.h"
#include <WiFi.h>
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
                  has_getKeyType<T>>
        {
        };

        template <typename T>
        constexpr bool is_secure_http_server_config_like_v = is_secure_http_server_config_like<T>::value;

        // Convenience alias for SFINAE
        template <typename T, typename = std::enable_if_t<is_secure_http_server_config_like_v<T>>>
        using enable_if_secure_http_server_config_t = void;

    } // namespace Restrictions



    template <uint32_t KEY_TYPE, size_t CACHE_SIZE = 8,
              typename = std::enable_if_t<(KEY_TYPE | BR_KEYTYPE_EC > 0) || (KEY_TYPE | BR_KEYTYPE_RSA > 0)>>
    class SecureHttpServerConfig
    {
    protected:
        mutable BearSSL::X509List *chain_;
        mutable BearSSL::PrivateKey *sk_;
        BearSSL::ServerSessions cache_;
        uint32_t keyType_;
        mutable bool initialized_;
        FS *fs_;
        const char *certPath_;
        const char *keyPath_;

        void lazyInitialize() const
        {
            if (initialized_)
                return;

            File certFile = fs_->open(certPath_, "r");
            String certContent;
            while (certFile.available())
            {
                certContent += static_cast<char>(certFile.read());
            }
            certFile.close();

            File keyFile = fs_->open(keyPath_, "r");
            String keyContent;
            while (keyFile.available())
            {
                keyContent += static_cast<char>(keyFile.read());
            }
            keyFile.close();

            chain_ = new BearSSL::X509List(certContent.c_str());
            sk_ = new BearSSL::PrivateKey(keyContent.c_str());
            initialized_ = true;
        }

    public:
        SecureHttpServerConfig(FS &fs, const char *certPath, const char *keyPath)
            : chain_(nullptr), sk_(nullptr), cache_(CACHE_SIZE), keyType_(KEY_TYPE),
              initialized_(false), fs_(&fs), certPath_(certPath), keyPath_(keyPath)
        {
        }
        SecureHttpServerConfig(const char *certContent, const char *keyContent)
            : chain_(new BearSSL::X509List(certContent)), sk_(new BearSSL::PrivateKey(keyContent)),
              cache_(CACHE_SIZE), keyType_(KEY_TYPE), initialized_(true), fs_(nullptr),
              certPath_(nullptr), keyPath_(nullptr)
        {
        }

        ~SecureHttpServerConfig()
        {
            if (initialized_)
            {
                delete chain_;
                delete sk_;
            }
        }

        BearSSL::ServerSessions *getCache() { return &cache_; }
        BearSSL::X509List *getCertificateChain()
        {
            lazyInitialize();
            return chain_;
        }
        BearSSL::PrivateKey *getPrivateKey()
        {
            lazyInitialize();
            return sk_;
        }
        uint32_t getKeyType() { return KEY_TYPE | BR_KEYTYPE_SIGN | BR_KEYTYPE_KEYX; }

        constexpr static uint32_t EC_KEY = BR_KEYTYPE_EC;
        constexpr static uint32_t RSA_KEY = BR_KEYTYPE_RSA;
    };

}