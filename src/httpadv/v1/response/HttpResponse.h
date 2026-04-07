#pragma once
#include <initializer_list>
#include "../transport/ByteStream.h"
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "ChunkedHttpResponseBodyStream.h"
#include "HttpResponseIterators.h"
#include "IHttpResponse.h"
#include <memory>
namespace httpadv::v1::response
{
  using httpadv::v1::core::HttpHeaderCollection;
  using httpadv::v1::core::HttpStatus;
  using lumalink::platform::buffers::IByteSource;

  class HttpResponse : public IHttpResponse
  {
  private:
    HttpStatus status_;
    HttpHeaderCollection headers_;
    std::unique_ptr<IByteSource> body_;

  public:
    HttpResponse(HttpStatus status, std::unique_ptr<IByteSource> body, HttpHeaderCollection &&headers);

    ~HttpResponse() override = default;

    HttpStatus status() const override;
    HttpHeaderCollection &headers() override;
    std::unique_ptr<IByteSource> getBody() override;
  };

  std::unique_ptr<IByteSource> CreateResponseStream(std::unique_ptr<IHttpResponse> response);

} // namespace httpadv::v1::response


