#include "HttpHandler.h"

namespace lumalink::http::handlers
{
    HttpHandler::HttpHandler(IHttpHandler::InvocationCallback invocation)
        : filter_(defaultFilter), invocation_(std::move(invocation))
    {
    }

    HttpHandler::HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const lumalink::http::core::HttpRequestContext &)> filter)
        : filter_(std::move(filter)), invocation_(std::move(invocation))
    {
    }

    HttpHandler::HttpHandler(std::unique_ptr<lumalink::http::response::IHttpResponse> response, std::function<bool(const lumalink::http::core::HttpRequestContext &)> filter)
        : filter_(std::move(filter)),
          invocation_([resp = std::make_shared<std::unique_ptr<lumalink::http::response::IHttpResponse>>(std::move(response))](lumalink::http::core::HttpRequestContext &) mutable -> IHttpHandler::HandlerResult
                      { return HandlerResult::responseResult(std::move(*resp)); })
    {
    }

    HttpHandler::HttpHandler(std::unique_ptr<lumalink::http::response::IHttpResponse> response)
        : filter_([](const lumalink::http::core::HttpRequestContext &)
                  { return true; }),
          invocation_([resp = std::make_shared<std::unique_ptr<lumalink::http::response::IHttpResponse>>(std::move(response))](lumalink::http::core::HttpRequestContext &) mutable -> IHttpHandler::HandlerResult
                      { return HandlerResult::responseResult(std::move(*resp)); })
    {
    }

    bool HttpHandler::defaultFilter(const lumalink::http::core::HttpRequestContext &context)
    {
        return context.completedPhases() == lumalink::http::core::HttpContextPhase::CompletedReadingHeaders + lumalink::http::core::HttpContextPhase::CompletedStartingLine;
    }

    HttpHandler::HandlerResult HttpHandler::handleStep(lumalink::http::core::HttpRequestContext &context)
    {
        if (filter_(context))
        {
            return invocation_(context);
        }
        return nullptr;
    }

    void HttpHandler::setPhaseFilter(lumalink::http::core::HttpContextPhaseFlags callAt)
    {
        filter_ = [callAt](const lumalink::http::core::HttpRequestContext &ctx)
        { return ctx.completedPhases() == callAt; };
    }
} // namespace lumalink::http::handlers
