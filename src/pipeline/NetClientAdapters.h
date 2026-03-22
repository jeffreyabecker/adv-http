#pragma once

#include "TransportInterfaces.h"

#include "../compat/IpAddress.h"

#include <memory>
#include <string>
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

        std::string_view remoteAddress() const override
        {
            remoteAddress_ = HttpServerAdvanced::IPAddress(connection_.remoteIP()).toString();
            return remoteAddress_;
        }

        std::uint16_t remotePort() override
        {
            return connection_.remotePort();
        }

        std::string_view localAddress() const override
        {
            localAddress_ = HttpServerAdvanced::IPAddress(connection_.localIP()).toString();
            return localAddress_;
        }

        std::uint16_t localPort() override
        {
            return connection_.localPort();
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
        mutable std::string remoteAddress_;
        mutable std::string localAddress_;
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