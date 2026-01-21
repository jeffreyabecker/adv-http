#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "./HttpServerBase.h"

namespace HttpServerAdvanced
{
    template <typename TServer = WiFiServer,  uint16_t DefaultPort = 80>
    class HttpServer : public HttpServerAdvanced::HttpServerBase
    {
    protected:

    public:
        HttpServer(uint16_t port = DefaultPort, const IPAddress &ip = IPAddress(IPADDR_ANY))
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

    protected:
        std::unique_ptr<IClient> accept() override
        {
            return server_.accept();
            // TClient client = server_.accept();
            // if (client)
            // {
            //     return std::make_unique<ClientWrapper>(std::make_unique<TClient>(std::move(client)));
            // }
            // return nullptr;
        }
        ServerImpl<TServer> server_;
    };

} // namespace HttpServerAdvanced
