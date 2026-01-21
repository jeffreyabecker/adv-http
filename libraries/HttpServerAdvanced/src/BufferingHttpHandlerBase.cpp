#include "./BufferingHttpHandlerBase.h"
#include "./Defines.h"
#include "./HttpHeader.h"
#include "./HttpRequest.h"
#include "./HttpRequestPhase.h"
#include <algorithm>
#include <limits>
#include <optional>

namespace HttpServerAdvanced
{

    IHttpHandler::HandlerResult BufferingHttpHandlerBase::handleStep(HttpRequest &context)
    {
        if (context.completedPhases() < HttpRequestPhase::CompletedReadingMessage)
        {
            return nullptr;
        }

        if (bodyBuffer_.empty())
        {
            return nullptr;
        }

        return handleBody(context, std::move(bodyBuffer_));
    }

    void BufferingHttpHandlerBase::handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length)
    {
        const ssize_t configuredMax = HttpServerAdvanced::MAX_BUFFERED_BODY_LENGTH;
        if (configuredMax == 0)
        {
            return;
        }

        const size_t maxBuffered = configuredMax < 0
                                       ? std::numeric_limits<size_t>::max()
                                       : static_cast<size_t>(configuredMax);

        if (contentLength_ == 0)
        {
            std::optional<HttpHeader> contentLengthHeader = context.headers().find("Content-Length");
            if (contentLengthHeader.has_value())
            {
                const long headerLength = contentLengthHeader->value().toInt();
                if (headerLength > 0)
                {
                    const size_t announcedLength = static_cast<size_t>(headerLength);
                    contentLength_ = std::min(announcedLength, maxBuffered);
                    bodyBuffer_.reserve(contentLength_);
                }
            }
        }

        const size_t spaceRemaining = maxBuffered > bodyBuffer_.size() ? maxBuffered - bodyBuffer_.size() : 0;
        if (spaceRemaining == 0)
        {
            return;
        }

        const size_t toCopy = std::min(length, spaceRemaining);
        bodyBuffer_.insert(bodyBuffer_.end(), at, at + toCopy);
        if (contentLength_ > 0 && bodyBuffer_.size() > contentLength_)
        {
            bodyBuffer_.resize(contentLength_);
        }

        if (contentLength_ > 0 && bodyBuffer_.size() == contentLength_)
        {
            handleBody(context, std::move(bodyBuffer_));
        }
    }

} // namespace HttpServerAdvanced
