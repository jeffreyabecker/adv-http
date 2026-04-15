#pragma once
#include <memory>
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "LumaLinkPlatform.h"
#include <span>
#include "ChunkedHttpResponseBodyStream.h"
#include "IHttpResponse.h"

namespace lumalink::http::response
{
  using lumalink::http::core::HttpHeader;
  using lumalink::http::core::HttpHeaderCollection;
  using lumalink::http::core::HttpHeaderNames;
  using lumalink::http::core::HttpStatus;
  using lumalink::platform::buffers::ConcatByteSource;
  using lumalink::platform::buffers::ByteAvailability;
  using lumalink::platform::buffers::ExhaustedResult;
  using lumalink::platform::buffers::IByteSource;
  using lumalink::platform::buffers::AvailableByteCount;
  using lumalink::platform::buffers::HasAvailableBytes;
  using lumalink::platform::buffers::IsExhausted;
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

    static bool responseHasNoBody(HttpStatus status);
    static std::string buildStartLine(HttpStatus status);
    static std::string buildHeadersBlock(const HttpHeaderCollection &headers);
    void buildSource();

  public:
    explicit HttpPipelineResponseSource(std::unique_ptr<IHttpResponse> response);

    ByteAvailability available() override
    {
      return source_ ? source_->available() : ExhaustedResult();
    }

    size_t read(std::span<uint8_t> buffer) override
    {
      return source_ ? source_->read(buffer) : 0;
    }

    size_t peek(std::span<uint8_t> buffer) override
    {
      return source_ ? source_->peek(buffer) : 0;
    }
  };

  inline std::unique_ptr<IByteSource> CreateResponseStream(std::unique_ptr<IHttpResponse> response)
  {
    return std::make_unique<HttpPipelineResponseSource>(std::move(response));
  }

} // namespace lumalink::http::response


