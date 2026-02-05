#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "HttpServerBase.h"

namespace HttpServerAdvanced
{
    template <typename TServer = WiFiServer,  uint16_t DEFAULT_PORT = 80>
    class StandardHttpServer : public HttpServerAdvanced::HttpServerBase
    {
    protected:

    public:
        static constexpr uint16_t DefaultPort = DEFAULT_PORT;
        StandardHttpServer(uint16_t port = DefaultPort, const IPAddress &ip = IPAddress(IPADDR_ANY))
            : server_(ip, port)
        {
        }

        void begin() override
        {
            HttpServerAdvanced::HttpServerBase::begin();
            server_.begin();
        }
        void end() override
        {
            server_.end();
        }
        void configureConnection(std::function<void(TServer *)> callback)
        {
            server_.configureConnection(callback);
        }
        virtual IPAddress localIP() const override
        {
            return WiFi.localIP();
        }
        virtual uint16_t localPort() const override
        {
            return server_.port();
        }

    protected:
        std::unique_ptr<IClient> accept() override
        {
            return server_.accept();
        }
        ServerImpl<TServer> server_;
    };

} // namespace HttpServerAdvanced

