#include "RawBodyHandler.h"
#include "../core/HttpHeader.h"
#include "../routing/HandlerMatcher.h"

#include <charconv>

namespace HttpServerAdvanced
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

    IHttpHandler::HandlerResult RawBodyHandler::handleStep(HttpRequest &context)
    {
        if (!response_ && context.completedPhases() >= HttpRequestPhase::CompletedReadingMessage)
        {
            handleBodyChunk(context, nullptr, 0);
        }
        if (response_)
        {
            return std::move(response_);
        }
        return nullptr;
    }

    void RawBodyHandler::handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length)
    {
        if (response_)
        {
            return;
        }
        if (receivedLength_ == 0)
        {
            params_ = extractor_(context);
            std::optional<HttpHeader> contentLengthHeader = context.headers().find(HttpHeaderNames::ContentLength);
            contentLength_ = contentLengthHeader.has_value() ? parseContentLength(contentLengthHeader->valueView()) : 0;
        }
        response_ = handler_(context, params_, RawBodyBuffer(receivedLength_, contentLength_, at, length));
        receivedLength_ += length;
    }

    RawBody::Invocation RawBody::curryWithoutParams(InvocationWithoutParams handler)
    {
        return [handler](HttpRequest &context, RouteParameters &, RawBodyBuffer buffer)
        {
            return handler(context, buffer);
        };
    }

    IHttpHandler::Factory RawBody::makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
    {
        return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
        {
            auto params = extractor(context);
            // Create a handler that adapts RawBodyBuffer to raw parameters
            std::function<IHttpHandler::HandlerResult(HttpRequest &, RouteParameters &, RawBodyBuffer)> bufferHandler =
                [handler](HttpRequest &ctx, RouteParameters &params, RawBodyBuffer buffer)
            {
                return handler(ctx, params, buffer);
            };
            return std::make_unique<RawBodyHandler>(bufferHandler, ExtractArgsFromRequest([params](HttpRequest &c)
                                                                                          { return params; }));
        };
    }

    RawBody::Invocation RawBody::curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](HttpRequest &context, RouteParameters &params, RawBodyBuffer buffer)
        {
            return interceptor(context, [handler, &params, buffer](HttpRequest &context)
                               { return handler(context, params, buffer); });
        };
    }

    RawBody::Invocation RawBody::applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](HttpRequest &context, RouteParameters &params, RawBodyBuffer buffer)
        {
            return interceptor(context, [handler, &params, &buffer](HttpRequest &context)
                               { return handler(context, params, buffer); });
        };
    }

    RawBody::Invocation RawBody::applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
    {
        return [filter, handler](HttpRequest &context, RouteParameters &params, RawBodyBuffer buffer)
        {
            auto response = handler(context, params, buffer);
            return filter(std::move(response));
        };
    }
} // namespace HttpServerAdvanced


