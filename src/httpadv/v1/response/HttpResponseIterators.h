#pragma once
#include <memory>
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../transport/ByteStream.h"
#include "ChunkedHttpResponseBodyStream.h"
#include "IHttpResponse.h"

namespace lumalink::http::response
{
  using lumalink::http::core::HttpHeader;
  using lumalink::http::core::HttpHeaderCollection;
  using lumalink::http::core::HttpHeaderNames;
  using lumalink::http::core::HttpStatus;
  using lumalink::platform::buffers::ConcatByteSource;
  using lumalink::platform::buffers::ExhaustedResult;
  using lumalink::platform::buffers::IByteSource;
  using lumalink::platform::buffers::AvailableResult;
  using lumalink::platform::buffers::SpanByteSource;
  using lumalink::platform::buffers::StdStringByteSource;

  // Helper functions
  std::string getHeaderDateValue();

  void EnsureRequiredHeaders(HttpHeaderCollection &headers, std::ptrdiff_t body_size);

  namespace ResponseStringConstants
  {
    constexpr const char CRLF[3] = "\r\n";
    constexpr const char START_LINE_DELIMITER[2] = " ";
    constexpr const char HEADER_DELIMITER[3] = ": ";
    constexpr const char HTTP_VERSION[10] = "HTTP/1.1 ";
  } // namespace ResponseStringConstants

  class HttpPipelineResponseSource : public IByteSource
  {
  private:
    std::unique_ptr<IHttpResponse> response_;
    std::unique_ptr<IByteSource> source_;

    static bool responseHasNoBody(HttpStatus status)
    {
      return status == HttpStatus::NoContent() ||
             status == HttpStatus::NotModified() ||
             (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
    }

    static std::string buildStartLine(HttpStatus status)
    {
      return std::string(ResponseStringConstants::HTTP_VERSION) +
             std::to_string(static_cast<uint16_t>(status)) +
             ResponseStringConstants::START_LINE_DELIMITER +
             status.toString() +
             ResponseStringConstants::CRLF;
    }

    static std::string buildHeadersBlock(const HttpHeaderCollection &headers)
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

    void buildSource()
    {
      std::unique_ptr<IByteSource> bodySource = response_->getBody();
      const AvailableResult bodyAvailable = bodySource ? bodySource->available() : ExhaustedResult();
        const std::ptrdiff_t knownBodySize = !bodySource ? 0 :
          (bodyAvailable.hasBytes() ? static_cast<std::ptrdiff_t>(bodyAvailable.count) :
          (bodyAvailable.isExhausted() ? 0 : -1));

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

  public:
    explicit HttpPipelineResponseSource(std::unique_ptr<IHttpResponse> response)
        : response_(std::move(response))
    {
      buildSource();
    }

    AvailableResult available() override
    {
      return source_ ? source_->available() : ExhaustedResult();
    }

    size_t read(lumalink::span<uint8_t> buffer) override
    {
      return source_ ? source_->read(buffer) : 0;
    }

    size_t peek(lumalink::span<uint8_t> buffer) override
    {
      return source_ ? source_->peek(buffer) : 0;
    }
  };

  inline std::unique_ptr<IByteSource> CreateResponseStream(std::unique_ptr<IHttpResponse> response)
  {
    return std::make_unique<HttpPipelineResponseSource>(std::move(response));
  }

} // namespace lumalink::http::response


