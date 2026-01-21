#pragma once
#include <memory>
#include <functional>
#include "./HttpMethod.h"
#include "./HttpContextPhase.h"
#include "./IHttpHandler.h"

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
        static bool defaultFilter(const HttpContext &context) { return context.completedPhases() == HttpContextPhase::CompletedReadingHeaders + HttpContextPhase::CompletedStartingLine; }

        HttpHandler(IHttpHandler::InvocationCallback invocation)
            : invocation_(invocation), filter_(defaultFilter) {}

        HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const HttpContext &)> filter)
            : invocation_(invocation), filter_(filter) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response)
            : invocation_([response = std::move(response)](HttpContext &context) mutable -> IHttpHandler::HandlerResult
                          { return std::move(response); }),
              filter_(defaultFilter) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response, std::function<bool(const HttpContext &)> filter)
            : invocation_([response = std::move(response)](HttpContext &context) mutable -> IHttpHandler::HandlerResult
                          { return std::move(response); }),
              filter_(filter) {}

        template <typename... Args>
        static std::unique_ptr<IHttpHandler> create(Args &&...args)
        {
            return std::make_unique<HttpHandler>(std::forward<Args>(args)...);
        }

        virtual ~HttpHandler() = default;

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
#include "./HttpContext.h"
