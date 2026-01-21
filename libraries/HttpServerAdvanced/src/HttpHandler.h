#pragma once
#include <memory>
#include <functional>



#include "./HttpMethod.h"
#include "./HttpContext.h"
#include "./IHttpHandler.h"

namespace HttpServerAdvanced
{

    class HttpHandler : public IHttpHandler
    {
    protected:
        static const HttpContextPhaseFlags CallHandleAt = HttpContextPhase::CompletedReadingHeaders + HttpContextPhase::CompletedStartingLine;
        std::function<bool(const HttpContext &)> filter_;
        IHttpHandler::InvocationCallback invocation_;

    public:
        static bool defaultFilter(const HttpContext &context) { return context.completedPhases() == HttpContextPhase::CompletedReadingHeaders + HttpContextPhase::CompletedStartingLine; }

        HttpHandler(IHttpHandler::InvocationCallback invocation)
            : invocation_(invocation), filter_(defaultFilter) {}

        HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const HttpContext &)> filter)
            : invocation_(invocation), filter_(filter) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response)
            : invocation_([response = std::move(response)](HttpContext &context) mutable -> IHttpHandler::HandlerResult
                          { return std::move(response); }),
              filter_(defaultFilter) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response, std::function<bool(const HttpContext &)> filter)
            : invocation_([response = std::move(response)](HttpContext &context) mutable -> IHttpHandler::HandlerResult
                          { return std::move(response); }),
              filter_(filter) {}

        template <typename... Args>
        static std::unique_ptr<IHttpHandler> create(Args &&...args)
        {
            return std::make_unique<HttpHandler>(std::forward<Args>(args)...);
        }

        virtual ~HttpHandler() = default;

        virtual HandlerResult handleStep(HttpContext &context) override
        {
            if (filter_(context))
            {
                return invocation_(context);
            }
            return nullptr;
        }

        virtual void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) override
        {
            // Default implementation does nothing
        }
    };

    class BufferingHttpHandlerBase : public IHttpHandler
    {
    private:
        std::vector<uint8_t> bodyBuffer_;
        size_t contentLength_{0};

    public:
        virtual ~BufferingHttpHandlerBase() = default;

        virtual HandlerResult handleBody(HttpContext &context, std::vector<uint8_t> &&body) = 0;
        HandlerResult handleStep(HttpContext &context) override
        {
            // Wait until the request body has been fully read
            if (context.completedPhases() < HttpContextPhase::CompletedReadingMessage)
            {
                return nullptr;
            }

            // If we already dispatched on reaching Content-Length, avoid double-call
            if (bodyBuffer_.empty())
            {
                return nullptr;
            }

            return handleBody(context, std::move(bodyBuffer_));
        }
        void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) override
        {
            if (contentLength_ == 0)
            {
                std::optional<HttpHeader> contentLengthHeader = context.request().headers().find("Content-Length");
                if (contentLengthHeader.has_value())
                {
                    contentLength_ = contentLengthHeader->value().toInt();
                    bodyBuffer_.reserve(contentLength_);
                }
            }

            // Always buffer incoming chunks; if content length is known clamp to that size
            bodyBuffer_.insert(bodyBuffer_.end(), at, at + length);
            if (contentLength_ > 0 && bodyBuffer_.size() > contentLength_)
            {
                bodyBuffer_.resize(contentLength_);
            }

            // If content length is known and we've reached it, process immediately
            if (contentLength_ > 0 && bodyBuffer_.size() == contentLength_)
            {
                handleBody(context, std::move(bodyBuffer_));
            }
        }
    };
}