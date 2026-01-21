#pragma once
#include <memory>
#include <functional>
#include "./HttpStatus.h"
#include "./HttpHeader.h"

namespace HttpServerAdvanced
{
  // Forward declarations
  class HttpResponseBodyStream;

  class IHttpResponse
  {
  public:
    using ResponseFilter = std::function<std::unique_ptr<IHttpResponse>(std::unique_ptr<IHttpResponse>)>;
    virtual ~IHttpResponse() = default;
    virtual HttpStatus status() const = 0;
    virtual HttpHeadersCollection &headers() = 0;
    virtual std::unique_ptr<HttpResponseBodyStream> getBody() = 0;
  };

} // namespace HttpServerAdvanced
