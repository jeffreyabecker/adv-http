#pragma once
#include <vector>
#include "IHttpHandler.h"
#include "../core/Defines.h"
#include "../core/HttpRequestPhase.h"
#include "../core/HttpHeader.h"
#include <charconv>
#include <limits>
#include <algorithm>
#include <optional>
#include <string_view>

namespace HttpServerAdvanced
{
    inline bool tryParseHeaderLength(std::string_view value, size_t &parsedLength)
    {
        parsedLength = 0;
        const char *begin = value.data();
        const char *end = begin + value.size();
        auto result = std::from_chars(begin, end, parsedLength);
        return result.ec == std::errc() && result.ptr == end;
    }

    // Forward declaration
    class HttpRequest;

    // Template base for handlers that buffer request bodies.
    // MaxBuffered defaults to the library-configured value `MAX_BUFFERED_BODY_LENGTH`.
    template <ssize_t MaxBuffered = HttpServerAdvanced::MAX_BUFFERED_BODY_LENGTH>
    class BufferingHttpHandlerBase : public IHttpHandler
    {
    private:
        std::vector<uint8_t> bodyBuffer_;
        size_t contentLength_{0};

        static constexpr ssize_t configuredMax = MaxBuffered;

    public:
        virtual ~BufferingHttpHandlerBase() = default;

        virtual HandlerResult handleBody(HttpRequest &context, std::vector<uint8_t> &&body) = 0;

        HandlerResult handleStep(HttpRequest &context) override
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

        void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length) override
        {
            const ssize_t cfg = configuredMax;
            if (cfg == 0)
            {
                return; // buffering disabled
            }

            const size_t maxBuffered = cfg < 0 ? std::numeric_limits<size_t>::max() : static_cast<size_t>(cfg);

            if (contentLength_ == 0)
            {
                std::optional<HttpHeader> contentLengthHeader = context.headers().find("Content-Length");
                if (contentLengthHeader.has_value())
                {
                    size_t headerLength = 0;
                    if (tryParseHeaderLength(contentLengthHeader->valueView(), headerLength) && headerLength > 0)
                    {
                        contentLength_ = std::min(headerLength, maxBuffered);
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
    };

    // Convenience alias with default template parameter
    using DefaultBufferingHttpHandlerBase = BufferingHttpHandlerBase<>;

} // namespace HttpServerAdvanced

