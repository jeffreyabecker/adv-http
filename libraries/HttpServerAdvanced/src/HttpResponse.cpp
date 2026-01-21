#include "./HttpResponse.h"
#include "./HttpHeader.h"
#include <algorithm>
#include "./Streams.h"
#include "./Iterators.h"

namespace HttpServerAdvanced
{

  void HttpResponse::init(size_t contentLength)
  {
    if (contentLength >= 0)
    {
      headers_.set(HttpHeader::ContentLength, String(contentLength));
      if (!headers_.exists(HttpHeader::ContentType))
      {
        headers_.set(HttpHeader::ContentType, defaultContentType);
      }
    }

    if (contentLength < 0)
    {
      headers_.remove(HttpHeader::ContentLength);
      headers_.set(HttpHeader::TransferEncoding, "chunked");
    }
  }

  // HttpResponse implementation
  HttpResponse::HttpResponse(HttpStatus status, String &&body, HttpHeadersCollection headers)
      : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body)))
  {
    init(body_->available());
  }

  HttpResponse::HttpResponse(HttpStatus status, const String &body, HttpHeadersCollection headers)
      : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body))
  {
    init(body_->available());
  }

  HttpResponse::HttpResponse(HttpStatus status, const String &body, HttpHeadersCollection headers)
      : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body))
  {
    init(body_->available());
  }
  HttpResponse::HttpResponse(HttpStatus status, const char *body, HttpHeadersCollection headers)
      : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body))
  {
    init(body_->available());
  }


  HttpStatus HttpResponse::status() const
  {
    return status_;
  }

  HttpHeadersCollection &HttpResponse::headers()
  {
    return headers_;
  }

  std::unique_ptr<HttpResponseBodyStream> HttpResponse::getBody()
  {
    return std::move(body_);
  }

  int HttpResponseBodyStream::available()
  {
    return innerStream_->available();
  }

  int HttpResponseBodyStream::read()
  {
    return innerStream_->read();
  }

  int HttpResponseBodyStream::peek()
  {
    return innerStream_->peek();
  }

  size_t HttpResponseBodyStream::write(uint8_t b)
  {
    return 0;
  }

  void ChunkedHttpResponseBodyStream::consume()
  {
    if (length_ > 0)
    {
      --length_;
      ++head_;
      if (length_ == 0)
      {
        head_ = 0;
      }
    }
  }

  constexpr std::size_t ChunkedHttpResponseBodyStream::hex_length(std::uint64_t n)
  {
    // At least one digit, even for 0.
    std::size_t len = 1;
    while (n >= 0x10)
    {
      n /= 0x10;
      ++len;
    }
    return len;
  }

  void ChunkedHttpResponseBodyStream::bufferNextChunk()
  {
    if (haveBufferedTerminalChunk_)
    {
      return;
    }

    size_t dataOffset = hex_length(HttpServerAdvanced::CHUNKED_RESPONSE_BUFFER_SIZE) + 3;          // +3 for \r\n and null terminator
    size_t maxInnerReadLength = HttpServerAdvanced::CHUNKED_RESPONSE_BUFFER_SIZE - dataOffset - 2; // -2 for trailing CRLF
    size_t bytesRead = 0;
    while (bytesRead < maxInnerReadLength)
    {
      int byte = innerStream_->read();
      if (byte == -1)
      {
        break;
      }
      buffer_[bytesRead] = static_cast<uint8_t>(byte);
      ++bytesRead;
    }
    if (bytesRead == 0)
    {
      // Buffer final chunk

      for (size_t i = 0; i < finalChunkLength; i++)
      {
        buffer_[i] = finalChunk[i];
      }
      length_ = finalChunkLength;
      head_ = 0;
      haveBufferedTerminalChunk_ = true;
      return;
    }
    else
    {
      /// @brief The number of characters written to the buffer by snprintf, excluding the null terminator.
      /// This call to snprintf is safe and will not overwrite data beyond the buffer size specified by dataOffset,
      /// as snprintf ensures it does not exceed the provided size limit. However, if dataOffset is incorrectly set
      /// (e.g., larger than the actual buffer capacity), it could potentially lead to undefined behavior or overwriting
      /// adjacent memory in practice, though the function itself prevents writing past the given size.
      auto printed = snprintf(reinterpret_cast<char *>(buffer_.data()), dataOffset, "%zx\r\n", bytesRead);
      auto toRemove = dataOffset - printed;
      if (toRemove > 0)
      {
        // Shift data to remove unused space
        for (size_t i = 0; i < bytesRead; ++i)
        {
          buffer_[printed + i] = buffer_[dataOffset + i];
        }
        length_ = printed + bytesRead + 2; // +2 for trailing CRLF
        head_ = 0;
      }
      else
      {
        length_ = dataOffset + bytesRead + 2; // +2 for trailing CRLF
        head_ = 0;
      }
    }
  }

  ChunkedHttpResponseBodyStream::ChunkedHttpResponseBodyStream(std::unique_ptr<Stream> innerStream)
      : HttpResponseBodyStream(std::move(innerStream))
  {
  }

  int ChunkedHttpResponseBodyStream::available()
  {
    if (done_)
    {
      return 0;
    }
    return -1;
  }

  int ChunkedHttpResponseBodyStream::read()
  {
    int result = peek();
    consume();
    return result;
  }

  int ChunkedHttpResponseBodyStream::peek()
  {
    if (done_)
    {
      return -1;
    }
    if (length_ == 0)
    {
      bufferNextChunk();
    }
    if (length_ == 0)
    {
      done_ = true;
      return -1;
    }
    return buffer_[head_];
  }

  size_t ChunkedHttpResponseBodyStream::write(uint8_t b)
  {
    return 0;
  }

  std::unique_ptr<HttpResponseBodyStream> ChunkedHttpResponseBodyStream::create(std::unique_ptr<Stream> innerStream)
  {
    return std::make_unique<ChunkedHttpResponseBodyStream>(std::move(innerStream));
  }

  String getHeaderDateValue()
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
  void EnsureRequiredHeaders(HttpHeadersCollection &headers, ssize_t body_size)
  {
    // Date header (RFC 7231 section 7.1.1.2 - MUST be sent by origin servers)
    if (!headers.exists(HttpHeader::Date))
    {
      headers.set(HttpHeader::Date, getHeaderDateValue());
    }
    if (!headers.exists(HttpHeader::Server))
    {
      headers.set(HttpHeader::Server, "Arduino-Pico");
    }
    if (!headers.exists(HttpHeader::ContentType))
    {
      headers.set(HttpHeader::ContentType, "text/plain");
    }
    if (!headers.exists(HttpHeader::Connection))
    {
      headers.set(HttpHeader::Connection, "close");
    }
    if(!headers.exists(HttpHeader::ContentLength) && 
       !headers.exists(HttpHeader::TransferEncoding))
    {
      if (body_size >= 0)
      {
        headers.set(HttpHeader::ContentLength, String(static_cast<uint16_t>(body_size)));
      }
      else
      {
        headers.set(HttpHeader::TransferEncoding, "chunked");
      }
    }

  }

  namespace ResponseStringConstants
  {
    const char CRLF[3] = "\r\n";
    const char START_LINE_DELIMITER[2] = " ";
    const char HEADER_DELIMITER[3] = ": ";
    const char HTTP_VERSION[10] = "HTTP/1.1 ";
  } // namespace Constants
  class HttpHeadersStartLineIterator : FixedStreamIterable<HttpHeadersStartLineIterator, 5>
  {
  public:
    using FixedStreamIterable<HttpHeadersStartLineIterator, 5>::begin;
    using FixedStreamIterable<HttpHeadersStartLineIterator, 5>::end;

  private:
    HttpStatus &status_;
    HttpHeadersStartLineIterator(size_t index, HttpStatus &status)
        : FixedStreamIterable<HttpHeadersStartLineIterator, 5>(index), status_(status) {}

  protected:
    value_type getAt(size_t index) override
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
  class HttpHeaderStreamIterator : FixedStreamIterable<HttpHeaderStreamIterator, 4>
  {
  private:
    HttpHeader &header_;
    HttpHeaderStreamIterator(size_t index, HttpHeader &header)
        : FixedStreamIterable<HttpHeaderStreamIterator, 4>(index), header_(header) {}

  protected:
    value_type getAt(size_t index) override
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
  class HttpHeadersCollectionStreamIterator : BoundedStreamIterable<HttpHeadersCollectionStreamIterator>
  {
  private:
    HttpHeadersCollection &headers_;
    HttpHeadersCollectionStreamIterator(size_t index, HttpHeadersCollection &headers)
        : BoundedStreamIterable<HttpHeadersCollectionStreamIterator>(index, headers.size()), headers_(headers) {}

  protected:
    value_type getAt(size_t index) override
    {
      return std::make_unique<IndefiniteConcatStream<HttpHeaderStreamIterator>>(HttpHeaderStreamIterator::begin(headers_[index]),
                                                                                HttpHeaderStreamIterator::end(headers_[index]));
    }

  public:
    static HttpHeadersCollectionStreamIterator begin(HttpHeadersCollection &headers)
    {
      return HttpHeadersCollectionStreamIterator(0, headers);
    }
    static HttpHeadersCollectionStreamIterator end(HttpHeadersCollection &headers)
    {
      return HttpHeadersCollectionStreamIterator(headers.size(), headers);
    }
  };

  class HttpPipelineResponseStream : public ConcatStream
  {
  private:
    std::unique_ptr<IHttpResponse> response_;
    std::unique_ptr<HttpResponseBodyStream> bodyStream_;

    std::unique_ptr<ReadStream> buildHeadersStream(HttpHeadersCollection &headers)
    {
    }
    std::unique_ptr<ReadStream> getStartLineStream()
    {
      return std::make_unique<IndefiniteConcatStream<HttpHeadersStartLineIterator>>(HttpHeadersStartLineIterator::begin(response_->status()),
                                                                                    HttpHeadersStartLineIterator::end(response_->status()));
    }
    std::unique_ptr<ReadStream> getHeadersStream()
    {
      bodyStream_ = response_->getBody();
      EnsureRequiredHeaders(response_->headers(), bodyStream_->available());
      return std::make_unique<IndefiniteConcatStream<HttpHeadersCollectionStreamIterator>>(HttpHeadersCollectionStreamIterator::begin(response_->headers()),
                                                                                           HttpHeadersCollectionStreamIterator::end(response_->headers()));
    }
    std::unique_ptr<ReadStream> getCrlfStream()
    {
      return std::make_unique<OctetsStream>(ResponseStringConstants::CRLF);
    }
    std::unique_ptr<ReadStream> getBodyStream()
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
        : response_(std::move(response)), ConcatStream({getStartLineStream(),
                                                        getCrlfStream(),
                                                        getHeadersStream(),
                                                        getCrlfStream(),
                                                        getBodyStream()})
    {
    }
  };
  std::unique_ptr<ReadStream> CreateResponseStream(std::unique_ptr<IHttpResponse> response)
  {
    return std::make_unique<HttpPipelineResponseStream>(std::move(response));
  }
} // namespace HttpServerAdvanced