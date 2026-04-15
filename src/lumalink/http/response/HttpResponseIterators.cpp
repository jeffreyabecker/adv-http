#include "HttpResponseIterators.h"
#include "../core/HttpHeaderCollection.h"
#include <ctime>
#include <cstring>

namespace lumalink::http::response
{
    bool HttpPipelineResponseSource::responseHasNoBody(HttpStatus status)
    {
        return status == HttpStatus::NoContent() ||
               status == HttpStatus::NotModified() ||
               (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
    }

    std::string HttpPipelineResponseSource::buildStartLine(HttpStatus status)
    {
        return std::string(ResponseStringConstants::HTTP_VERSION) +
               std::to_string(static_cast<uint16_t>(status)) +
               ResponseStringConstants::START_LINE_DELIMITER +
               status.toString() +
               ResponseStringConstants::CRLF;
    }

    std::string HttpPipelineResponseSource::buildHeadersBlock(const HttpHeaderCollection &headers)
    {
        std::string result;
        for (const HttpHeader &header : headers)
        {
            result.append(header.nameView().data(), header.nameView().size());
            result.append(ResponseStringConstants::HEADER_DELIMITER);
            result.append(header.valueView().data(), header.valueView().size());
            result.append(ResponseStringConstants::CRLF);
        }
        return result;
    }

    void HttpPipelineResponseSource::buildSource()
    {
        std::unique_ptr<IByteSource> bodySource = response_->getBody();
        const ByteAvailability bodyAvailable = bodySource ? bodySource->available() : ExhaustedResult();
        const std::ptrdiff_t knownBodySize = !bodySource ? 0 :
            (HasAvailableBytes(bodyAvailable) ? static_cast<std::ptrdiff_t>(AvailableByteCount(bodyAvailable)) :
             (IsExhausted(bodyAvailable) ? 0 : -1));

        EnsureRequiredHeaders(response_->headers(), knownBodySize);

        if (responseHasNoBody(response_->status()))
        {
            bodySource.reset();
        }
        else if (response_->headers().exists(HttpHeaderNames::TransferEncoding, "chunked") && bodySource)
        {
            bodySource = ChunkedHttpResponseBodyStream::create(std::move(bodySource));
        }

        std::vector<std::unique_ptr<IByteSource>> sources;
        sources.emplace_back(std::make_unique<StdStringByteSource>(buildStartLine(response_->status())));
        sources.emplace_back(std::make_unique<StdStringByteSource>(buildHeadersBlock(response_->headers())));
        sources.emplace_back(std::make_unique<SpanByteSource>(std::string_view(ResponseStringConstants::CRLF, 2)));

        if (bodySource)
        {
            sources.emplace_back(std::move(bodySource));
        }

        source_ = std::make_unique<ConcatByteSource>(std::move(sources));
    }

    HttpPipelineResponseSource::HttpPipelineResponseSource(std::unique_ptr<IHttpResponse> response)
        : response_(std::move(response))
    {
        buildSource();
    }

    std::string getHeaderDateValue()
    {
        // This assumes the system time is correctly set using the NTP system
        struct tm tm_time;
        const time_t now = time(nullptr);
    #ifdef _WIN32
        gmtime_s(&tm_time, &now);
    #else
        gmtime_r(&now, &tm_time);
    #endif

        char buf[32];
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm_time);
        return std::string(buf);
    }

    void EnsureRequiredHeaders(HttpHeaderCollection &headers, std::ptrdiff_t body_size)
    {
        // Date header (RFC 7231 section 7.1.1.2 - MUST be sent by origin servers)
        if (!headers.exists(HttpHeaderNames::Date))
        {
            headers.set(HttpHeaderNames::Date, getHeaderDateValue());
        }
        if (!headers.exists(HttpHeaderNames::Server))
        {
            headers.set(HttpHeaderNames::Server, "Arduino-Pico");
        }
        if (!headers.exists(HttpHeaderNames::ContentType))
        {
            headers.set(HttpHeaderNames::ContentType, "text/plain");
        }
        if (!headers.exists(HttpHeaderNames::Connection))
        {
            headers.set(HttpHeaderNames::Connection, "close");
        }
        if(!headers.exists(HttpHeaderNames::ContentLength) && 
           !headers.exists(HttpHeaderNames::TransferEncoding))
        {
            if (body_size >= 0)
            {
                headers.set(HttpHeaderNames::ContentLength, std::to_string(static_cast<size_t>(body_size)));
            }
            else
            {
                headers.set(HttpHeaderNames::TransferEncoding, "chunked");
            }
        }
    }
} // namespace lumalink::http::response

