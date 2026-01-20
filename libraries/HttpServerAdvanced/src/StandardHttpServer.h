#pragma once

#include "./pipeline/Pipeline.h"

#include <WifiServer.h>

namespace HttpServerAdvanced
{
    template <typename T = WiFiServer, uint16_t DefaultPort = 80>
    class HttpServer : public HttpServerAdvanced::Pipeline::HttpServerBase
    {
    public:
        HttpServer(uint16_t port = DefaultPort, const IPAddress &ip = IPAddress(IPADDR_ANY))
            : server_(ip, port)
        {
        }

        void begin() override
        {
            HttpServerAdvanced::Pipeline::HttpServerBase::begin();
            server_.begin();
        }
        void end() override
        {
            server_.end();
        }
        void configureConnection(std::function<void(T *)> callback)
        {
            server_.configureConnection(callback);
        }

    protected:
        std::unique_ptr<Stream> accept() override
        {
            return server_.accept();
        }
        ServerImpl<T> server_;
    };

  

} // namespace HttpServerAdvanced
