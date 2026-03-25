#pragma once
#include <memory>
#include <functional>
#include "../core/HttpMethod.h"
#include "../core/HttpRequestPhase.h"
#include "IHttpHandler.h"

namespace HttpServerAdvanced
{
    // Forward declaration
    class HttpContext;

    class HttpHandler : public IHttpHandler
    {
    protected:
        static const HttpRequestPhaseFlags CallHandleAt = HttpRequestPhase::CompletedReadingHeaders + HttpRequestPhase::CompletedStartingLine;
        std::function<bool(const HttpContext &)> filter_;
        IHttpHandler::InvocationCallback invocation_;

    public:
        static bool defaultFilter(const HttpContext &context);

        HttpHandler(IHttpHandler::InvocationCallback invocation)
            : invocation_(invocation), filter_(defaultFilter) {}

        HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const HttpContext &)> filter)
            : invocation_(invocation), filter_(filter) {}


        HttpHandler(std::unique_ptr<IHttpResponse> response, std::function<bool(const HttpContext &)> filter)
            : invocation_([resp = std::make_shared<std::unique_ptr<IHttpResponse>>(std::move(response))](HttpContext &) mutable -> IHttpHandler::HandlerResult
                          { return HandlerResult::responseResult(std::move(*resp)); }),
              filter_(filter) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response)
            : invocation_([resp = std::make_shared<std::unique_ptr<IHttpResponse>>(std::move(response))](HttpContext &) mutable -> IHttpHandler::HandlerResult
                          { return HandlerResult::responseResult(std::move(*resp)); }),
              filter_([](const HttpContext & req) { return true; }) {} // will be set via setPhaseFilter

        template <typename... Args>
        static std::unique_ptr<IHttpHandler> create(Args &&...args)
        {
            return std::make_unique<HttpHandler>(std::forward<Args>(args)...);
        }

        virtual ~HttpHandler() = default;

        void setPhaseFilter(HttpRequestPhaseFlags callAt);

        virtual HandlerResult handleStep(HttpContext &context) override
        {
            if (filter_(context))
            {
                return invocation_(context);
            }
            return nullptr;
        }

        virtual void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) override
        {
            // Default implementation does nothing
        }
    };

}

// Include HttpContext after class definition to resolve circular dependency
#include "../core/HttpContext.h"

namespace HttpServerAdvanced {
    inline bool HttpHandler::defaultFilter(const HttpContext &context) {
        return context.completedPhases() == HttpRequestPhase::CompletedReadingHeaders + HttpRequestPhase::CompletedStartingLine;
    }

    // Helper for setting filter when using phase-based constructor: call after construction
    inline void HttpHandler::setPhaseFilter(HttpRequestPhaseFlags callAt) {
        filter_ = [callAt](const HttpContext &ctx) { return ctx.completedPhases() == callAt; };
    }
}



