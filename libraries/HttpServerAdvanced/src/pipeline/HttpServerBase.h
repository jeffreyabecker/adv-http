#pragma once
#include "../util/Util.h"
#include "./HttpPipeline.h"
#include "./HttpPipelineTimeouts.h"

#include <memory>
#include <list>
#include <vector>
#include <map>
#include <functional>

namespace HttpServerAdvanced::Pipeline
{
    class IHttpPipelineHandlerFactory
    {
    public:
        static constexpr const char *ServiceName = "HttpPipelineHandlerFactory";
        virtual ~IHttpPipelineHandlerFactory() = default;
        virtual std::unique_ptr<IHttpPipelineHandler> createHandler(HttpPipeline &pipeline) = 0;
    };
    

    class HttpServerBase : public ServiceManager<HttpServerBase>
    {
    public:
        virtual ~HttpServerBase();

        void handleClient();

        virtual void begin() {}
        virtual void end() {};

        HttpPipelineTimeouts &timeouts();
        void setTimeouts(const HttpPipelineTimeouts &timeouts);
        void setPipelineHandlerFactory(IHttpPipelineHandlerFactory *factory);

    protected:
        IHttpPipelineHandlerFactory *pipelineHandlerFactory_;
        std::unique_ptr<HttpPipeline> currentPipeline_;
        HttpPipelineTimeouts timeouts_;

        virtual std::unique_ptr<Stream> accept() = 0;
    };
} // namespace HttpServerAdvanced