#pragma once
#include <Arduino.h>
#include <optional>
#include "./IHttpHandler.h"
#include "./core/HttpHeader.h"
#include "./core/HttpHeaderCollection.h"
#include "./core/Buffer.h"
#include "./HandlerRestrictions.h"

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

        virtual HandlerResult handleStep(HttpRequest &context);
        virtual void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length);
    };
    class RawBody
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, RawBodyBuffer)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &, RawBodyBuffer)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler);

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor);

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler);
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };
} // namespace HttpServerAdvanced
