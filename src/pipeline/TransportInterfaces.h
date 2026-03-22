#pragma once

#include "../streams/ByteStream.h"
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
}