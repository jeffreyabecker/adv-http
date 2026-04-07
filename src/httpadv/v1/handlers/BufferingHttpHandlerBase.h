#pragma once
#include <vector>
#include "IHttpHandler.h"
#include "../core/Defines.h"
#include "../core/HttpContextPhase.h"
#include "../core/HttpHeader.h"
#include <charconv>
#include <limits>
#include <algorithm>
#include <optional>
#include <string_view>
#include "../core/HttpRequestContext.h"

namespace httpadv::v1::handlers
{
    using httpadv::v1::core::HttpHeader;

    inline bool tryParseHeaderLength(std::string_view value, size_t &parsedLength)
    {
        parsedLength = 0;
        const char *begin = value.data();
        const char *end = begin + value.size();
        auto result = std::from_chars(begin, end, parsedLength);
        return result.ec == std::errc() && result.ptr == end;
    }

    // Template base for handlers that buffer request bodies.
    // MaxBuffered defaults to the library-configured value `MAX_BUFFERED_BODY_LENGTH`.
    template <std::ptrdiff_t MaxBuffered = httpadv::v1::core::MAX_BUFFERED_BODY_LENGTH>
    class BufferingHttpHandlerBase : public IHttpHandler
    {
    private:
        std::vector<uint8_t> bodyBuffer_;
        size_t contentLength_{0};

        static constexpr std::ptrdiff_t configuredMax = MaxBuffered;

    public:
        virtual ~BufferingHttpHandlerBase() = default;

        virtual HandlerResult handleBody(httpadv::v1::core::HttpRequestContext &context, std::vector<uint8_t> &&body) = 0;

        HandlerResult handleStep(httpadv::v1::core::HttpRequestContext &context) override
        {
            if (context.completedPhases() < httpadv::v1::core::HttpContextPhase::CompletedReadingMessage)
            {
                return nullptr;
            }

            if (bodyBuffer_.empty())
            {
                return nullptr;
            }

            return handleBody(context, std::move(bodyBuffer_));
        }

        void handleBodyChunk(httpadv::v1::core::HttpRequestContext &context, const uint8_t *at, std::size_t length) override
        {
            const std::ptrdiff_t cfg = configuredMax;
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

} // namespace httpadv::v1::handlers

