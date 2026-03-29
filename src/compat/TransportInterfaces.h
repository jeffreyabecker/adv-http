#pragma once

#include "ByteStream.h"
#include "../core/Defines.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace HttpServerAdvanced
{
    class IClient : public IByteChannel
    {
    public:
        ~IClient() override = default;

        virtual void stop() = 0;
        virtual bool connected() = 0;
        virtual std::string_view remoteAddress() const = 0;
        virtual std::uint16_t remotePort() = 0;
        virtual std::string_view localAddress() const = 0;
        virtual std::uint16_t localPort() = 0;
        virtual void setTimeout(std::uint32_t timeoutMs) = 0;
        virtual std::uint32_t getTimeout() const = 0;
    };

    class IServer
    {
    public:
        using ClientType = IClient;

        virtual ~IServer() = default;

        IServer() = default;

        virtual std::unique_ptr<IClient> accept() = 0;
        virtual void begin() = 0;
        virtual std::uint16_t port() const = 0;
        virtual std::string_view localAddress() const = 0;
        virtual void end() = 0;
    };

    class IPeer
    {
    public:
        virtual ~IPeer() = default;

        virtual bool begin(std::uint16_t port) = 0;
        virtual bool beginMulticast(std::string_view multicastAddress, std::uint16_t port) = 0;

        virtual void stop() = 0;

        virtual bool beginPacket(std::string_view address, std::uint16_t port) = 0;
        virtual bool endPacket() = 0;

        virtual std::size_t write(HttpServerAdvanced::span<const std::uint8_t> buffer) = 0;

        virtual AvailableResult parsePacket() = 0;
        virtual AvailableResult available() = 0;
        virtual std::size_t read(HttpServerAdvanced::span<std::uint8_t> buffer) = 0;
        virtual std::size_t peek(HttpServerAdvanced::span<std::uint8_t> buffer) = 0;
        virtual void flush() = 0;

        virtual std::string_view remoteAddress() const = 0;
        virtual std::uint16_t remotePort() const = 0;
    };

    class ITransportFactory
    {
    public:
        virtual ~ITransportFactory() = default;

        virtual std::unique_ptr<IServer> createServer(std::uint16_t port) = 0;
        virtual std::unique_ptr<IClient> createClient(std::string_view address, std::uint16_t port) = 0;
        virtual std::unique_ptr<IPeer> createPeer() = 0;
    };
}