#pragma once
#include <memory>
#include <functional>
#include "../core/HttpMethod.h"
#include "../core/HttpRequestPhase.h"
#include "IHttpHandler.h"

namespace HttpServerAdvanced
{
    // Forward declaration
    class HttpRequest;

    class HttpHandler : public IHttpHandler
    {
    protected:
        static const HttpRequestPhaseFlags CallHandleAt = HttpRequestPhase::CompletedReadingHeaders + HttpRequestPhase::CompletedStartingLine;
        std::function<bool(const HttpRequest &)> filter_;
        IHttpHandler::InvocationCallback invocation_;

    public:
        static bool defaultFilter(const HttpRequest &context);

        HttpHandler(IHttpHandler::InvocationCallback invocation)
            : invocation_(invocation), filter_(defaultFilter) {}

        HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const HttpRequest &)> filter)
            : invocation_(invocation), filter_(filter) {}


        HttpHandler(std::unique_ptr<IHttpResponse> response, std::function<bool(const HttpRequest &)> filter)
            : invocation_([resp = std::make_shared<std::unique_ptr<IHttpResponse>>(std::move(response))](HttpRequest &) mutable -> IHttpHandler::HandlerResult
                          { return std::move(*resp); }),
              filter_(filter) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response)
            : invocation_([resp = std::make_shared<std::unique_ptr<IHttpResponse>>(std::move(response))](HttpRequest &) mutable -> IHttpHandler::HandlerResult
                          { return std::move(*resp); }),
              filter_([](const HttpRequest & req) { return true; }) {} // will be set via setPhaseFilter

        template <typename... Args>
        static std::unique_ptr<IHttpHandler> create(Args &&...args)
        {
            return std::make_unique<HttpHandler>(std::forward<Args>(args)...);
        }

        virtual ~HttpHandler() = default;

        void setPhaseFilter(HttpRequestPhaseFlags callAt);

        virtual HandlerResult handleStep(HttpRequest &context) override
        {
            if (filter_(context))
            {
                return invocation_(context);
            }
            return nullptr;
        }

        virtual void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length) override
        {
            // Default implementation does nothing
        }
    };

}

// Include HttpRequest after class definition to resolve circular dependency
#include "../server/HttpRequest.h"

namespace HttpServerAdvanced {
    inline bool HttpHandler::defaultFilter(const HttpRequest &context) {
        return context.completedPhases() == HttpRequestPhase::CompletedReadingHeaders + HttpRequestPhase::CompletedStartingLine;
    }

    // Helper for setting filter when using phase-based constructor: call after construction
    inline void HttpHandler::setPhaseFilter(HttpRequestPhaseFlags callAt) {
        filter_ = [callAt](const HttpRequest &ctx) { return ctx.completedPhases() == callAt; };
    }
}


