#pragma once
#include <memory>
#include <functional>
#include "../core/HttpMethod.h"
#include "../core/HttpContextPhase.h"
#include "IHttpHandler.h"

namespace httpadv::v1::handlers
{
    class HttpHandler : public IHttpHandler
    {
    protected:
        static const httpadv::v1::core::HttpContextPhaseFlags CallHandleAt = httpadv::v1::core::HttpContextPhase::CompletedReadingHeaders + httpadv::v1::core::HttpContextPhase::CompletedStartingLine;
        std::function<bool(const httpadv::v1::core::HttpContext &)> filter_;
        IHttpHandler::InvocationCallback invocation_;

    public:
        static bool defaultFilter(const httpadv::v1::core::HttpContext &context);

        HttpHandler(IHttpHandler::InvocationCallback invocation);

        HttpHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &)> invocation);

        HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const httpadv::v1::core::HttpContext &)> filter);

        HttpHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &)> invocation, std::function<bool(const httpadv::v1::core::HttpContext &)> filter);


        HttpHandler(std::unique_ptr<httpadv::v1::response::IHttpResponse> response, std::function<bool(const httpadv::v1::core::HttpContext &)> filter)
                        : filter_(filter),
                            invocation_([resp = std::make_shared<std::unique_ptr<httpadv::v1::response::IHttpResponse>>(std::move(response))](httpadv::v1::core::HttpRequestContext &) mutable -> IHttpHandler::HandlerResult
                                                    { return HandlerResult::responseResult(std::move(*resp)); }) {}

        HttpHandler(std::unique_ptr<httpadv::v1::response::IHttpResponse> response)
                        : filter_([](const httpadv::v1::core::HttpContext & req) { return true; }),
                            invocation_([resp = std::make_shared<std::unique_ptr<httpadv::v1::response::IHttpResponse>>(std::move(response))](httpadv::v1::core::HttpRequestContext &) mutable -> IHttpHandler::HandlerResult
                                                    { return HandlerResult::responseResult(std::move(*resp)); }) {} // will be set via setPhaseFilter

        template <typename... Args>
        static std::unique_ptr<IHttpHandler> create(Args &&...args)
        {
            return std::make_unique<HttpHandler>(std::forward<Args>(args)...);
        }

        virtual ~HttpHandler() = default;

        void setPhaseFilter(httpadv::v1::core::HttpContextPhaseFlags callAt);

        virtual HandlerResult handleStep(httpadv::v1::core::HttpContext &context) override;

        virtual void handleBodyChunk(httpadv::v1::core::HttpContext &context, const uint8_t *at, std::size_t length) override
        {
            // Default implementation does nothing
        }
    };

}

// Include httpadv::v1::core::HttpContext after class definition to resolve circular dependency
#include "../core/HttpContext.h"

namespace httpadv::v1::handlers {
    inline HttpHandler::HttpHandler(IHttpHandler::InvocationCallback invocation)
        : filter_(defaultFilter), invocation_(std::move(invocation)) {}

    inline HttpHandler::HttpHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &)> invocation)
        : filter_(defaultFilter),
          invocation_([invocation = std::move(invocation)](httpadv::v1::core::HttpRequestContext &context) mutable
                      { return invocation(static_cast<httpadv::v1::core::HttpContext &>(context)); }) {}

    inline HttpHandler::HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const httpadv::v1::core::HttpContext &)> filter)
        : filter_(std::move(filter)), invocation_(std::move(invocation)) {}

    inline HttpHandler::HttpHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &)> invocation, std::function<bool(const httpadv::v1::core::HttpContext &)> filter)
        : filter_(std::move(filter)),
          invocation_([invocation = std::move(invocation)](httpadv::v1::core::HttpRequestContext &context) mutable
                      { return invocation(static_cast<httpadv::v1::core::HttpContext &>(context)); }) {}

    inline bool HttpHandler::defaultFilter(const httpadv::v1::core::HttpContext &context) {
        return context.completedPhases() == httpadv::v1::core::HttpContextPhase::CompletedReadingHeaders + httpadv::v1::core::HttpContextPhase::CompletedStartingLine;
    }

    inline HttpHandler::HandlerResult HttpHandler::handleStep(httpadv::v1::core::HttpContext &context) {
        if (filter_(context)) {
            return invocation_(context);
        }
        return nullptr;
    }

    // Helper for setting filter when using phase-based constructor: call after construction
    inline void HttpHandler::setPhaseFilter(httpadv::v1::core::HttpContextPhaseFlags callAt) {
        filter_ = [callAt](const httpadv::v1::core::HttpContext &ctx) { return ctx.completedPhases() == callAt; };
    }
}



