#pragma once
#include <Arduino.h>
#include <optional>
#include "./IHttpHandler.h"
#include "./HttpHeader.h"
#include "./Buffer.h"

namespace HttpServerAdvanced
{
    class RawBodyBuffer : public Buffer
    {
    private:
        size_t receivedLength_{0};
        size_t contentLength_{0};

    public:
        RawBodyBuffer(size_t receivedLength, size_t contentLength, const uint8_t *data, size_t size)
            : Buffer(data, size), receivedLength_(receivedLength), contentLength_(contentLength) {}
        size_t receivedLength() const { return receivedLength_; }
        size_t contentLength() const { return contentLength_; }
    };

    class RawBodyHandler : public IHttpHandler
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &, RawBodyBuffer)> handler_;
        ExtractArgsFromRequest extractor_;
        HandlerResult response_;
        std::vector<String> params_;
        size_t receivedLength_{0};
        size_t contentLength_{0};

    public:
        RawBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &, RawBodyBuffer)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        RawBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, RawBodyBuffer)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequest &context, std::vector<String> &, RawBodyBuffer buffer)
                       { return handler(context, buffer); }),
              extractor_(extractor) {}

        virtual HandlerResult handleStep(HttpRequest &context)
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
        virtual void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length)
        {
            if (response_)
            {
                return;
            }
            if (receivedLength_ == 0)
            {
                params_ = extractor_(context);
                std::optional<HttpHeader> contentLengthHeader = context.headers().find(HttpHeader::ContentLength);
                contentLength_ = contentLengthHeader.has_value() ? contentLengthHeader->value().toInt() : 0;
            }
            response_ = handler_(context, params_, RawBodyBuffer(receivedLength_, contentLength_, at, length));
            receivedLength_ += length;
        }
    };
    class RawBody
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, RawBodyBuffer)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &, RawBodyBuffer)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpRequest &context, std::vector<String> &, RawBodyBuffer buffer)
            {
                return handler(context, buffer);
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
        {
            return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                // Create a handler that adapts RawBodyBuffer to raw parameters
                std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &, RawBodyBuffer)> bufferHandler =
                    [handler](HttpRequest &ctx, std::vector<String> &params, RawBodyBuffer buffer)
                {
                    return handler(ctx, params, buffer);
                };
                return std::make_unique<RawBodyHandler>(bufferHandler, ExtractArgsFromRequest([params](HttpRequest &c)
                                                                                              { return params; }));
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &params, RawBodyBuffer buffer)
            {
                return interceptor(context, [handler, &params, buffer](HttpRequest &context)
                                   { return handler(context, params, buffer); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &params, RawBodyBuffer buffer)
            {
                return interceptor(context, [handler, &params, &buffer](HttpRequest &context)
                                   { return handler(context, params, buffer); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpRequest &context, std::vector<String> &params, RawBodyBuffer buffer)
            {
                auto response = handler(context, params, buffer);
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };
} // namespace HttpServerAdvanced
