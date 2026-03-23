#pragma once
#include <Arduino.h>
#include <optional>
#include <utility>
#include <vector>
#include "IHttpHandler.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"

namespace HttpServerAdvanced
{
    class RawBodyBuffer
    {
    private:
        std::vector<uint8_t> data_;
        size_t receivedLength_{0};
        size_t contentLength_{0};

    public:
        RawBodyBuffer(size_t receivedLength, size_t contentLength, const uint8_t *data, size_t size)
            : data_(), receivedLength_(receivedLength), contentLength_(contentLength)
        {
            if (data != nullptr && size > 0)
            {
                data_.assign(data, data + size);
            }
        }
        RawBodyBuffer(size_t receivedLength, size_t contentLength, std::vector<uint8_t> data)
            : data_(std::move(data)), receivedLength_(receivedLength), contentLength_(contentLength) {}
        const uint8_t *data() const { return data_.data(); }
        size_t size() const { return data_.size(); }
        const uint8_t *begin() const { return data_.data(); }
        const uint8_t *end() const { return data_.data() + data_.size(); }
        const std::vector<uint8_t> &bytes() const { return data_; }
        size_t receivedLength() const { return receivedLength_; }
        size_t contentLength() const { return contentLength_; }
    };

    class RawBodyHandler : public IHttpHandler
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequest &, RouteParameters &, RawBodyBuffer)> handler_;
        ExtractArgsFromRequest extractor_;
        HandlerResult response_;
        RouteParameters params_;
        size_t receivedLength_{0};
        size_t contentLength_{0};

    public:
        RawBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, RouteParameters &, RawBodyBuffer)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        RawBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, RawBodyBuffer)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequest &context, RouteParameters &, RawBodyBuffer buffer)
                       { return handler(context, buffer); }),
              extractor_(extractor) {}

        virtual HandlerResult handleStep(HttpRequest &context);
        virtual void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length);
    };
    class RawBody
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, RawBodyBuffer)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, RouteParameters &, RawBodyBuffer)>;

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

