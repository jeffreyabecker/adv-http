#pragma once
#include "../core/Defines.h"
#include "StandardHttpServer.h"
#include <WiFi.h>
#include <type_traits>
#include <utility>
namespace HttpServerAdvanced
{

    class ISecureHttpServerConfig
    {
    public:
        virtual ~ISecureHttpServerConfig() = default;

        virtual BearSSL::ServerSessions *getCache() = 0;
        virtual BearSSL::X509List *getCertificateChain() = 0;
        virtual BearSSL::PrivateKey *getPrivateKey() = 0;
        virtual uint32_t getKeyType() = 0;
    };

    template <uint32_t KEY_TYPE, size_t CACHE_SIZE = 8,
              typename = std::enable_if_t<(KEY_TYPE | BR_KEYTYPE_EC > 0) || (KEY_TYPE | BR_KEYTYPE_RSA > 0)>>
    class SecureHttpServerConfig : public ISecureHttpServerConfig
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

        BearSSL::ServerSessions *getCache() override { return &cache_; }
        BearSSL::X509List *getCertificateChain() override
        {
            lazyInitialize();
            return chain_;
        }
        BearSSL::PrivateKey *getPrivateKey() override
        {
            lazyInitialize();
            return sk_;
        }
        uint32_t getKeyType() override { return KEY_TYPE | BR_KEYTYPE_SIGN | BR_KEYTYPE_KEYX; }

        constexpr static uint32_t EC_KEY = BR_KEYTYPE_EC;
        constexpr static uint32_t RSA_KEY = BR_KEYTYPE_RSA;
    };

}

