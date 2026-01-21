#pragma once
#include <vector>
#include "./IHttpHandler.h"

namespace HttpServerAdvanced
{
    // Forward declaration
    class HttpContext;

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

// Include HttpContext after class definition to resolve circular dependency
#include "./HttpContext.h"
