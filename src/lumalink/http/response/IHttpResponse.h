#pragma once
#include <memory>
#include <functional>
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "LumaLinkPlatform.h"

namespace lumalink::http::response
{
  using lumalink::http::core::HttpHeaderCollection;
  using lumalink::http::core::HttpStatus;
  using lumalink::platform::buffers::IByteSource;

  class IHttpResponse
  {
  public:
    using ResponseFilter = std::function<std::unique_ptr<IHttpResponse>(std::unique_ptr<IHttpResponse>)>;
    virtual ~IHttpResponse() = default;
    virtual HttpStatus status() const = 0;
    virtual HttpHeaderCollection &headers() = 0;
    virtual std::unique_ptr<IByteSource> getBody() = 0;
  };

} // namespace lumalink::http::response

