#include "RawBodyHandler.h"
#include "../core/HttpHeader.h"
#include "../routing/HandlerMatcher.h"

#include <charconv>

namespace httpadv::v1::handlers
{
    namespace
    {
        size_t parseContentLength(std::string_view value)
        {
            size_t parsedValue = 0;
            const char *begin = value.data();
            const char *end = begin + value.size();
            const auto result = std::from_chars(begin, end, parsedValue);
            return (result.ec == std::errc() && result.ptr == end) ? parsedValue : 0;
        }
    }

    IHttpHandler::HandlerResult RawBodyHandler::handleStep(httpadv::v1::core::HttpContext &context)
    {
        if (!response_ && context.completedPhases() >= httpadv::v1::core::HttpContextPhase::CompletedReadingMessage)
        {
            handleBodyChunk(context, nullptr, 0);
        }
        if (response_)
        {
            return std::move(response_);
        }
        return nullptr;
    }

    void RawBodyHandler::handleBodyChunk(httpadv::v1::core::HttpContext &context, const uint8_t *at, std::size_t length)
    {
        if (response_)
        {
            return;
        }
        if (receivedLength_ == 0)
        {
            params_ = extractor_(context);
            std::optional<httpadv::v1::core::HttpHeader> contentLengthHeader = context.headers().find(httpadv::v1::core::HttpHeaderNames::ContentLength);
            contentLength_ = contentLengthHeader.has_value() ? parseContentLength(contentLengthHeader->valueView()) : 0;
        }
        response_ = handler_(context, params_, RawBodyBuffer(receivedLength_, contentLength_, at, length));
        receivedLength_ += length;
    }

    RawBody::Invocation RawBody::curryWithoutParams(InvocationWithoutParams handler)
    {
        return [handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &, RawBodyBuffer buffer)
        {
            return handler(context, buffer);
        };
    }

    IHttpHandler::Factory RawBody::makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
    {
        return [handler, extractor](httpadv::v1::core::HttpContext &context) -> std::unique_ptr<IHttpHandler>
        {
            auto params = extractor(context);
            // Create a handler that adapts RawBodyBuffer to raw parameters
            std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpRequestContext &, RouteParameters &, RawBodyBuffer)> bufferHandler =
                [handler](httpadv::v1::core::HttpRequestContext &ctx, RouteParameters &params, RawBodyBuffer buffer)
            {
                return handler(ctx, params, buffer);
            };
            return std::make_unique<RawBodyHandler>(bufferHandler, ExtractArgsFromRequest([params](httpadv::v1::core::HttpContext &c)
                                                                                          { return params; }));
        };
    }

    RawBody::Invocation RawBody::curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &params, RawBodyBuffer buffer)
        {
            return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, &params, buffer]() mutable
                               { return handler(context, params, buffer); }));
        };
    }

    RawBody::Invocation RawBody::applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &params, RawBodyBuffer buffer)
        {
            return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, &params, &buffer]() mutable
                               { return handler(context, params, buffer); }));
        };
    }

    RawBody::Invocation RawBody::applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
    {
        return [filter, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &params, RawBodyBuffer buffer)
        {
            auto response = handler(context, params, buffer);
            if (!response.isResponse())
            {
                return response;
            }

            response.response = filter(std::move(response.response));
            return response;
        };
    }
} // namespace HttpServerAdvanced


