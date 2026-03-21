#pragma once

#include "TransportInterfaces.h"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace HttpServerAdvanced
{
    template <typename T>
    class ClientImpl : public IClient
    {
    public:
        explicit ClientImpl(T client)
            : connection_(std::move(client))
        {
        }

        ~ClientImpl() override
        {
            if (connection_ && connection_.connected())
            {
                connection_.stop();
            }
        }

        std::size_t write(const std::uint8_t *buf, std::size_t size) override
        {
            return connection_.write(buf, size);
        }

        int available() override
        {
            return connection_.available();
        }

        int read(std::uint8_t *rbuf, std::size_t size) override
        {
            return connection_.read(rbuf, size);
        }

        void flush() override
        {
            connection_.flush();
        }

        void stop() override
        {
            connection_.stop();
        }

        ConnectionStatus status() override
        {
            return static_cast<ConnectionStatus>(connection_.status());
        }

        std::uint8_t connected() override
        {
            return connection_.connected();
        }

        IPAddress remoteIP() override
        {
            return connection_.remoteIP();
        }

        std::uint16_t remotePort() override
        {
            return connection_.remotePort();
        }

        IPAddress localIP() override
        {
            return connection_.localIP();
        }

        std::uint16_t localPort() override
        {
            return connection_.localPort();
        }

        void configureConnection(std::function<void(T *)> callback)
        {
            if (callback)
            {
                callback(&connection_);
            }
        }

        void setTimeout(std::uint32_t timeoutMs) override
        {
            connection_.setTimeout(timeoutMs);
        }

        std::uint32_t getTimeout() const override
        {
            return const_cast<T &>(connection_).getTimeout();
        }

    private:
        T connection_;
    };

    template <typename T>
    class ServerImpl : public IServer
    {
    public:
        template <typename... Args>
        ServerImpl(Args &&...args)
            : connection_(std::forward<Args>(args)...)
        {
        }

        ~ServerImpl() override = default;

        std::unique_ptr<IClient> accept() override
        {
            auto client = connection_.accept();
            if (client)
            {
                using ClientType = std::decay_t<decltype(client)>;
                return std::make_unique<ClientImpl<ClientType>>(std::move(client));
            }

            return nullptr;
        }

        void configureConnection(std::function<void(T *)> callback)
        {
            if (callback)
            {
                callback(&connection_);
            }
        }

        void begin() override
        {
            connection_.begin();
        }

        ConnectionStatus status() override
        {
            return static_cast<ConnectionStatus>(connection_.status());
        }

        std::uint16_t port() const override
        {
            return connection_.port();
        }

        void end() override
        {
            connection_.end();
        }

    private:
        T connection_;
    };
}