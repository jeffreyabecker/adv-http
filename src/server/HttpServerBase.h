#pragma once

#include "../compat/Clock.h"
#include "../compat/IpAddress.h"
#include "../pipeline/HttpPipeline.h"
#include "../pipeline/IPipelineHandler.h"
#include "../pipeline/PipelineHandleClientResult.h"
#include "../core/HttpTimeouts.h"

#include <memory>
#include <list>
#include <vector>
#include <map>
#include <functional>
#include <any>

namespace HttpServerAdvanced
{

    class HttpServerBase
    {
    public:
        HttpServerBase();
        virtual ~HttpServerBase();

        void handleClient();

        virtual void begin();
        virtual void end();

        HttpTimeouts &timeouts();
        void setTimeouts(const HttpTimeouts &timeouts);
        void setClock(const Compat::Clock &clock);
        const Compat::Clock &clock() const;


        void setPipelineHandlerFactory(std::function<PipelineHandlerPtr(HttpServerBase &)> factory);

        // Accessors for concurrent connection limits
        size_t activeConnections() const { return pipelines_.size(); }
        static constexpr size_t maxConcurrentConnections() { return MAX_CONCURRENT_CONNECTIONS; }

        virtual IPAddress localIP() const = 0;
        virtual uint16_t localPort() const = 0;

    protected:
        std::function<PipelineHandlerPtr(HttpServerBase &)> pipelineHandlerFactory_;
        // Multiple concurrent pipelines (one per accepted client)
        std::vector<std::unique_ptr<HttpPipeline>> pipelines_;
        HttpTimeouts timeouts_;
        const Compat::Clock *clock_;
        mutable std::map<String, std::any> items_;

        virtual std::unique_ptr<IClient> accept() = 0;
    };

} // namespace HttpServerAdvanced


