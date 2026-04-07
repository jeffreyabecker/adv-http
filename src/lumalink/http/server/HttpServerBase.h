#pragma once

#include "../util/Clock.h"
#include "../pipeline/HttpPipeline.h"
#include "../pipeline/IPipelineHandler.h"
#include "../pipeline/PipelineHandleClientResult.h"
#include "LumaLinkPlatform.h"
#include "../core/HttpTimeouts.h"

#include <memory>
#include <list>
#include <vector>
#include <map>
#include <functional>
#include <any>
#include <string_view>

namespace lumalink::http::server
{
    using lumalink::http::core::MAX_CONCURRENT_CONNECTIONS;
    using lumalink::http::core::HttpTimeouts;
    using lumalink::http::pipeline::HttpPipeline;
    using lumalink::http::pipeline::PipelineHandlerPtr;
    using lumalink::platform::transport::IServer;
    using lumalink::http::util::Clock;

    class HttpServerBase
    {
    public:
        explicit HttpServerBase(std::unique_ptr<IServer> server);
        virtual ~HttpServerBase();

        void handleClient();

        virtual void begin();
        virtual void end();

        HttpTimeouts &timeouts();
        void setTimeouts(const HttpTimeouts &timeouts);
        void setClock(const Clock &clock);
        const Clock &clock() const;


        void setPipelineHandlerFactory(std::function<PipelineHandlerPtr(HttpServerBase &)> factory);

        // Accessors for concurrent connection limits
        size_t activeConnections() const { return pipelines_.size(); }
        static constexpr size_t maxConcurrentConnections() { return MAX_CONCURRENT_CONNECTIONS; }

        virtual std::string_view localAddress() const;
        virtual uint16_t localPort() const;

    protected:
        std::function<PipelineHandlerPtr(HttpServerBase &)> pipelineHandlerFactory_;
        std::unique_ptr<IServer> server_;
        // Multiple concurrent pipelines (one per accepted client)
        std::vector<std::unique_ptr<HttpPipeline>> pipelines_;
        HttpTimeouts timeouts_;
        const Clock *clock_;
        mutable std::map<std::string, std::any> items_;
    };

} // namespace lumalink::http::server


