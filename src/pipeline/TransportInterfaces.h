#pragma once

#include "TransportEndpoint.h"
#include "../compat/Availability.h"
#include "../core/Defines.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace HttpServerAdvanced
{
    enum class ConnectionStatus
    {
        CLOSED = 0,
        LISTEN = 1,
        SYN_SENT = 2,
        SYN_RCVD = 3,
        ESTABLISHED = 4,
        FIN_WAIT_1 = 5,
        FIN_WAIT_2 = 6,
        CLOSE_WAIT = 7,
        CLOSING = 8,
        LAST_ACK = 9,
        TIME_WAIT = 10
    };

    class IClient
    {
    public:
        virtual ~IClient() = default;

        virtual std::size_t write(const std::uint8_t *buf, std::size_t size) = 0;
        virtual int available() = 0;
        virtual int read(std::uint8_t *rbuf, std::size_t size) = 0;
        virtual void flush() = 0;
        virtual void stop() = 0;
        virtual ConnectionStatus status() = 0;
        virtual std::uint8_t connected() = 0;
        virtual IPAddress remoteIP() = 0;
        virtual std::uint16_t remotePort() = 0;
        virtual IPAddress localIP() = 0;
        virtual std::uint16_t localPort() = 0;
        virtual void setTimeout(std::uint32_t timeoutMs) = 0;
        virtual std::uint32_t getTimeout() const = 0;

        virtual AvailableResult availability()
        {
            const int result = available();
            if (result > 0)
            {
                return AvailableBytes(static_cast<std::size_t>(result));
            }

            if (result == 0)
            {
                return ExhaustedResult();
            }

            return TemporarilyUnavailableResult();
        }

        virtual TransportEndpoint remoteEndpoint()
        {
            return TransportEndpoint(remoteIP(), remotePort());
        }

        virtual TransportEndpoint localEndpoint()
        {
            return TransportEndpoint(localIP(), localPort());
        }
    };

    class IServer
    {
    public:
        using ClientType = IClient;

        virtual ~IServer() = default;

        IServer() = default;

        explicit IServer(const TransportEndpoint &endpoint)
        {
            (void)endpoint;
        }

        IServer(const IPAddress &ip, std::uint16_t port)
            : IServer(TransportEndpoint(ip, port))
        {
        }

        virtual std::unique_ptr<IClient> accept() = 0;
        virtual void begin() = 0;
        virtual ConnectionStatus status() = 0;
        virtual std::uint16_t port() const = 0;
        virtual void end() = 0;

        virtual TransportEndpoint localEndpoint() const
        {
            return TransportEndpoint(IPAddress(), port());
        }
    };
}