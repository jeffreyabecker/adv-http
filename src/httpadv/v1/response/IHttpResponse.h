#pragma once
#include <memory>
#include <functional>
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../compat/ByteStream.h"

namespace httpadv::v1::response
{
  using httpadv::v1::core::HttpHeaderCollection;
  using httpadv::v1::core::HttpStatus;
  using httpadv::v1::transport::IByteSource;

  class IHttpResponse
  {
  public:
    using ResponseFilter = std::function<std::unique_ptr<IHttpResponse>(std::unique_ptr<IHttpResponse>)>;
    virtual ~IHttpResponse() = default;
    virtual HttpStatus status() const = 0;
    virtual HttpHeaderCollection &headers() = 0;
    virtual std::unique_ptr<IByteSource> getBody() = 0;
  };

} // namespace httpadv::v1::response

