#pragma once
#include <memory>
#include <functional>
#include <utility>
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


        HttpHandler(std::unique_ptr<lumalink::http::response::IHttpResponse> response, std::function<bool(const lumalink::http::core::HttpRequestContext &)> filter);

        HttpHandler(std::unique_ptr<lumalink::http::response::IHttpResponse> response);

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



