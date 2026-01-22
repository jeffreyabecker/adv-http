#pragma once
#include <memory>
#include "./HttpStatus.h"
#include "./HttpHeader.h"
#include "./Streams.h"
#include "./Iterators.h"
#include "./HttpResponseBodyStream.h"
#include "./IHttpResponse.h"

namespace HttpServerAdvanced
{

  // Helper functions
  inline String getHeaderDateValue()
  {
    // This assumes the system time is correctly set using the NTP system
    struct tm tm_time;
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t now = tv.tv_sec;
    gmtime_r(&now, &tm_time);

    char buf[32];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm_time);
    return String(buf);
  }

  inline void EnsureRequiredHeaders(HttpHeadersCollection &headers, ssize_t body_size)
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
        headers.set(HttpHeaderNames::ContentLength, String(static_cast<size_t>(body_size)));
      }
      else
      {
        headers.set(HttpHeaderNames::TransferEncoding, "chunked");
      }
    }
  }

  namespace ResponseStringConstants
  {
    constexpr const char CRLF[3] = "\r\n";
    constexpr const char START_LINE_DELIMITER[2] = " ";
    constexpr const char HEADER_DELIMITER[3] = ": ";
    constexpr const char HTTP_VERSION[10] = "HTTP/1.1 ";
  } // namespace ResponseStringConstants

  class HttpHeadersStartLineIterator : public FixedStreamIterable<HttpHeadersStartLineIterator, 5>
  {
  public:
    using FixedStreamIterable<HttpHeadersStartLineIterator, 5>::begin;
    using FixedStreamIterable<HttpHeadersStartLineIterator, 5>::end;

  private:
    const HttpStatus status_;
  public:
    HttpHeadersStartLineIterator(size_t index, HttpStatus status)
        : FixedStreamIterable<HttpHeadersStartLineIterator, 5>(index), status_(status) {}

  protected:
    value_type getAt(size_t index) const override
    {
      switch (index)
      {
      case 0:
        return std::make_unique<OctetsStream>(ResponseStringConstants::HTTP_VERSION);
      case 1:
        return std::make_unique<OctetsStream>(String(static_cast<uint16_t>(status_)));
      case 2:
        return std::make_unique<OctetsStream>(ResponseStringConstants::START_LINE_DELIMITER);
      case 3:
        return std::make_unique<OctetsStream>(status_.toString());
      case 4:
        return std::make_unique<OctetsStream>(ResponseStringConstants::CRLF);
      default:
        return nullptr;
      }
    }
  };

  class HttpHeaderStreamIterator : public FixedStreamIterable<HttpHeaderStreamIterator, 4>
  {
  private:
    const HttpHeader header_;
  public:
    HttpHeaderStreamIterator(size_t index, HttpHeader header)
        : FixedStreamIterable<HttpHeaderStreamIterator, 4>(index), header_(header) {}

  protected:
    value_type getAt(size_t index) const override
    {
      switch (index)
      {
      case 0:
        return std::make_unique<OctetsStream>(header_.name());
      case 1:
        return std::make_unique<OctetsStream>(ResponseStringConstants::HEADER_DELIMITER);
      case 2:
        return std::make_unique<OctetsStream>(header_.value());
      case 3:
        return std::make_unique<OctetsStream>(ResponseStringConstants::CRLF);
      default:
        return nullptr;
      }
    }

  public:
    using FixedStreamIterable<HttpHeaderStreamIterator, 4>::begin;
    using FixedStreamIterable<HttpHeaderStreamIterator, 4>::end;
  };

  class HttpHeadersCollectionStreamIterator : public BoundedStreamIterable<HttpHeadersCollectionStreamIterator>
  {
  private:
    const HttpHeadersCollection &headers_;
  public:
    HttpHeadersCollectionStreamIterator(size_t index, const HttpHeadersCollection &headers)
        : BoundedStreamIterable<HttpHeadersCollectionStreamIterator>(index, headers.size()), headers_(headers) {}

  protected:
    value_type getAt(size_t index) const override
    {
      return std::make_unique<IndefiniteConcatStream<HttpHeaderStreamIterator>>(HttpHeaderStreamIterator::begin(headers_[index]),
                                                                                HttpHeaderStreamIterator::end(headers_[index]));
    }

  public:
    static HttpHeadersCollectionStreamIterator begin(const HttpHeadersCollection &headers)
    {
      return HttpHeadersCollectionStreamIterator(0, headers);
    }
    static HttpHeadersCollectionStreamIterator end(const HttpHeadersCollection &headers)
    {
      return HttpHeadersCollectionStreamIterator(headers.size(), headers);
    }
  };



  class HttpPipelineResponseStream : public ConcatStream<5>
  {
  private:
    std::unique_ptr<IHttpResponse> response_;
    std::unique_ptr<HttpResponseBodyStream> bodyStream_;

    std::unique_ptr<Stream> getStartLineStream()
    {
      return std::make_unique<IndefiniteConcatStream<HttpHeadersStartLineIterator>>(HttpHeadersStartLineIterator::begin(response_->status()),
                                                                                    HttpHeadersStartLineIterator::end(response_->status()));
    }
    std::unique_ptr<Stream> getHeadersStream()
    {
      bodyStream_ = response_->getBody();
      EnsureRequiredHeaders(response_->headers(), bodyStream_->available());
      return std::make_unique<IndefiniteConcatStream<HttpHeadersCollectionStreamIterator>>(HttpHeadersCollectionStreamIterator::begin(response_->headers()),
                                                                                           HttpHeadersCollectionStreamIterator::end(response_->headers()));
    }
    std::unique_ptr<Stream> getCrlfStream()
    {
      return std::make_unique<OctetsStream>(ResponseStringConstants::CRLF);
    }
    std::unique_ptr<Stream> getBodyStream()
    {
      if (response_->status() == HttpStatus::NoContent() ||
          response_->status() == HttpStatus::NotModified() ||
          (static_cast<int>(response_->status()) >= 100 && static_cast<int>(response_->status()) < 200))
      {
        return std::make_unique<EmptyReadStream>(); // No body for these statuses
      }
      return std::move(bodyStream_);
    }

  public:
    HttpPipelineResponseStream(std::unique_ptr<IHttpResponse> response)
        : response_(std::move(response)), ConcatStream<5>(std::array<std::unique_ptr<Stream>, 5>{
                                                        getStartLineStream(),
                                                        getCrlfStream(),
                                                        getHeadersStream(),
                                                        getCrlfStream(),
                                                        getBodyStream()})
    {
    }
  };

  inline std::unique_ptr<Stream> CreateResponseStream(std::unique_ptr<IHttpResponse> response)
  {
    return std::make_unique<HttpPipelineResponseStream>(std::move(response));
  }

} // namespace HttpServerAdvanced
