#pragma once
#include <memory>
#include <functional>
#include "../core/HttpMethod.h"
#include "../core/HttpContextPhase.h"
#include "IHttpHandler.h"

namespace HttpServerAdvanced
{
    // Forward declaration
    class HttpContext;

    class HttpHandler : public IHttpHandler
    {
    protected:
        static const HttpContextPhaseFlags CallHandleAt = HttpContextPhase::CompletedReadingHeaders + HttpContextPhase::CompletedStartingLine;
        std::function<bool(const HttpContext &)> filter_;
        IHttpHandler::InvocationCallback invocation_;

    public:
        static bool defaultFilter(const HttpContext &context);

        HttpHandler(IHttpHandler::InvocationCallback invocation)
            : filter_(defaultFilter), invocation_(invocation) {}

        HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const HttpContext &)> filter)
            : filter_(filter), invocation_(invocation) {}


        HttpHandler(std::unique_ptr<IHttpResponse> response, std::function<bool(const HttpContext &)> filter)
                        : filter_(filter),
                            invocation_([resp = std::make_shared<std::unique_ptr<IHttpResponse>>(std::move(response))](HttpContext &) mutable -> IHttpHandler::HandlerResult
                                                    { return HandlerResult::responseResult(std::move(*resp)); }) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response)
                        : filter_([](const HttpContext & req) { return true; }),
                            invocation_([resp = std::make_shared<std::unique_ptr<IHttpResponse>>(std::move(response))](HttpContext &) mutable -> IHttpHandler::HandlerResult
                                                    { return HandlerResult::responseResult(std::move(*resp)); }) {} // will be set via setPhaseFilter

        template <typename... Args>
        static std::unique_ptr<IHttpHandler> create(Args &&...args)
        {
            return std::make_unique<HttpHandler>(std::forward<Args>(args)...);
        }

        virtual ~HttpHandler() = default;

        void setPhaseFilter(HttpContextPhaseFlags callAt);

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
        return context.completedPhases() == HttpContextPhase::CompletedReadingHeaders + HttpContextPhase::CompletedStartingLine;
    }

    // Helper for setting filter when using phase-based constructor: call after construction
    inline void HttpHandler::setPhaseFilter(HttpContextPhaseFlags callAt) {
        filter_ = [callAt](const HttpContext &ctx) { return ctx.completedPhases() == callAt; };
    }
}



