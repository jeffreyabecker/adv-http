#pragma once
#include <memory>
#include <functional>
#include "../core/HttpMethod.h"
#include "../core/HttpContextPhase.h"
#include "../core/HttpRequestContext.h"
#include "IHttpHandler.h"

namespace lumalink::http::handlers
{
    class HttpHandler : public IHttpHandler
    {
    protected:
        static const lumalink::http::core::HttpContextPhaseFlags CallHandleAt = lumalink::http::core::HttpContextPhase::CompletedReadingHeaders + lumalink::http::core::HttpContextPhase::CompletedStartingLine;
        std::function<bool(const lumalink::http::core::HttpRequestContext &)> filter_;
        IHttpHandler::InvocationCallback invocation_;

    public:
        static bool defaultFilter(const lumalink::http::core::HttpRequestContext &context);

        HttpHandler(IHttpHandler::InvocationCallback invocation);

        HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const lumalink::http::core::HttpRequestContext &)> filter);


        HttpHandler(std::unique_ptr<lumalink::http::response::IHttpResponse> response, std::function<bool(const lumalink::http::core::HttpRequestContext &)> filter)
                        : filter_(filter),
                            invocation_([resp = std::make_shared<std::unique_ptr<lumalink::http::response::IHttpResponse>>(std::move(response))](lumalink::http::core::HttpRequestContext &) mutable -> IHttpHandler::HandlerResult
                                                    { return HandlerResult::responseResult(std::move(*resp)); }) {}

        HttpHandler(std::unique_ptr<lumalink::http::response::IHttpResponse> response)
                        : filter_([](const lumalink::http::core::HttpRequestContext & req) { return true; }),
                            invocation_([resp = std::make_shared<std::unique_ptr<lumalink::http::response::IHttpResponse>>(std::move(response))](lumalink::http::core::HttpRequestContext &) mutable -> IHttpHandler::HandlerResult
                                                    { return HandlerResult::responseResult(std::move(*resp)); }) {} // will be set via setPhaseFilter

        template <typename... Args>
        static std::unique_ptr<IHttpHandler> create(Args &&...args)
        {
            return std::make_unique<HttpHandler>(std::forward<Args>(args)...);
        }

        virtual ~HttpHandler() = default;

        void setPhaseFilter(lumalink::http::core::HttpContextPhaseFlags callAt);

        virtual HandlerResult handleStep(lumalink::http::core::HttpRequestContext &context) override;

        virtual void handleBodyChunk(lumalink::http::core::HttpRequestContext &context, const uint8_t *at, std::size_t length) override
        {
            // Default implementation does nothing
        }
    };

}

namespace lumalink::http::handlers {
    inline HttpHandler::HttpHandler(IHttpHandler::InvocationCallback invocation)
        : filter_(defaultFilter), invocation_(std::move(invocation)) {}

    inline HttpHandler::HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const lumalink::http::core::HttpRequestContext &)> filter)
        : filter_(std::move(filter)), invocation_(std::move(invocation)) {}

    inline bool HttpHandler::defaultFilter(const lumalink::http::core::HttpRequestContext &context) {
        return context.completedPhases() == lumalink::http::core::HttpContextPhase::CompletedReadingHeaders + lumalink::http::core::HttpContextPhase::CompletedStartingLine;
    }

    inline HttpHandler::HandlerResult HttpHandler::handleStep(lumalink::http::core::HttpRequestContext &context) {
        if (filter_(context)) {
            return invocation_(context);
        }
        return nullptr;
    }

    // Helper for setting filter when using phase-based constructor: call after construction
    inline void HttpHandler::setPhaseFilter(lumalink::http::core::HttpContextPhaseFlags callAt) {
        filter_ = [callAt](const lumalink::http::core::HttpRequestContext &ctx) { return ctx.completedPhases() == callAt; };
    }
}



