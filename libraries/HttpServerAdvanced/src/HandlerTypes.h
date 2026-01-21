#pragma once
#include <Arduino.h>
#include "./Defines.h"
#include "./HttpHandler.h"
#include "./HttpResponse.h"
#include "./HandlerRestrictions.h"
#include "./HandlerMatcher.h"
#include "./HttpHandlerFactory.h"
#include "./KeyValuePairView.h"
namespace HttpServerAdvanced
{
    class NoBodyHandler;
    class Request
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpContext &)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpContext &, std::vector<String> &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpContext &context, std::vector<String> &&)
            {
                return handler(context);
            };
        }

        static HttpHandlerFactory::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
        {
            return [handler, extractor](HttpContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<NoBodyHandler>(handler, [params](HttpContext &c)
                                                       { return params; });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &&params)
            {
                return interceptor(context, [handler, &params](HttpContext &context)
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &&params)
            {
                return interceptor(context, [handler, &params](HttpContext &context)
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpContext &context, std::vector<String> &&params)
            {
                auto response = handler(context, std::move(params));
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };
    class FormBodyHandler;
    class Form
    {
    public:
        using PostBodyData = KeyValuePairView<String, String>;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpContext &, PostBodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpContext &, std::vector<String> &&, PostBodyData &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpContext &context, std::vector<String> &&, PostBodyData &&postData)
            {
                return handler(context, std::move(postData));
            };
        }

        static HttpHandlerFactory::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
        {
            return [handler, extractor](HttpContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<FormBodyHandler>(handler, [params](HttpContext &c)
                                                         { return params; });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                return interceptor(context, [handler, &params, &postData](HttpContext &context)
                                   { return handler(context, std::move(params), std::move(postData)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                return interceptor(context, [handler, &params, &postData](HttpContext &context)
                                   { return handler(context, std::move(params), std::move(postData)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpContext &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                auto response = handler(context, std::move(params), std::move(postData));
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };
    class RawBodyHandler;
    class RawBody
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpContext &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpContext &, std::vector<String> &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpContext &context, std::vector<String> &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return handler(context, recievedLength, contentLength, chunk, chunkLength);
            };
        }

        static HttpHandlerFactory::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
        {
            return [handler, extractor](HttpContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<RawBodyHandler>(handler, [params](HttpContext &c)
                                                        { return params; });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return interceptor(context, [handler, &params, &recievedLength, &contentLength, &chunk, &chunkLength](HttpContext &context)
                                   { return handler(context, params, recievedLength, contentLength, chunk, chunkLength); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return interceptor(context, [handler, &params, &recievedLength, &contentLength, &chunk, &chunkLength](HttpContext &context)
                                   { return handler(context, params, recievedLength, contentLength, chunk, chunkLength); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpContext &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                auto response = handler(context, params, recievedLength, contentLength, chunk, chunkLength);
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };

    // Handler Implementations

    class NoBodyHandler : public HttpHandler
    {
    private:
        // Request::Invocation handler_;
        // ParameterExtractor extractor_;

    public:
        NoBodyHandler(Request::Invocation handler, ParameterExtractor extractor)
            : HttpHandler([handler, extractor](HttpContext &context) -> IHttpHandler::HandlerResult
                          {
                                    auto params = extractor(context);
                                    return handler(context, std::move(params)); }) {}
        NoBodyHandler(Request::InvocationWithoutParams handler, ParameterExtractor extractor)
            : HttpHandler([handler](HttpContext &context) -> IHttpHandler::HandlerResult
                          { return handler(context); }) {}
    };

    class FormBodyHandler : public BufferingHttpHandlerBase
    {
    private:
        Form::Invocation handler_;
        ParameterExtractor extractor_;

    public:
        FormBodyHandler(Form::Invocation handler, ParameterExtractor extractor)
            : handler_(handler), extractor_(extractor) {}
        FormBodyHandler(Form::InvocationWithoutParams handler, ParameterExtractor extractor)
            : handler_(Form::curryWithoutParams(handler)), extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(HttpContext &context, std::vector<uint8_t> &&body) override
        {
            auto params = extractor_(context);

            KeyValuePairView<String, String> postData = HttpUtility::ParseQueryString(reinterpret_cast<const char *>(body.data()), body.size());
            return handler_(context, std::move(params), std::move(postData));
        }
    };

    class RawBodyHandler : public IHttpHandler
    {
    private:
        RawBody::Invocation handler_;
        ParameterExtractor extractor_;
        HandlerResult response_;
        std::vector<String> params_;
        size_t receivedLength_{0};
        size_t contentLength_{0};

    public:
        RawBodyHandler(RawBody::Invocation handler, ParameterExtractor extractor)
            : handler_(handler), extractor_(extractor) {}
        RawBodyHandler(RawBody::InvocationWithoutParams handler, ParameterExtractor extractor)
            : handler_(RawBody::curryWithoutParams(handler)), extractor_(extractor) {}

        virtual HandlerResult handleStep(HttpContext &context)
        {
            if (!response_ && context.completedPhases() >= HttpContextPhase::CompletedReadingMessage)
            {
                handleBodyChunk(context, nullptr, 0);
            }
            if (response_)
            {
                return std::move(response_);
            }
            return nullptr;
        }
        virtual void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length)
        {
            if (response_)
            {
                return;
            }
            if (receivedLength_ == 0)
            {
                params_ = extractor_(context);
                std::optional<HttpHeader> contentLengthHeader = context.request().headers().find(HttpHeader::ContentLength);
                contentLength_ = contentLengthHeader.has_value() ? contentLengthHeader->value().toInt() : 0;
            }
            response_ = handler_(context, params_, receivedLength_, contentLength_, at, length);
            receivedLength_ += length;
        }
    };

} // namespace HttpServerAdvanced
