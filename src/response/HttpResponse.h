#pragma once
#include <initializer_list>
#include <cstring>
#include "../streams/ByteStream.h"
#include "../compat/Stream.h"
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../streams/Streams.h"
#include "../streams/Iterators.h"
#include "HttpResponseBodyStream.h"
#include "ChunkedHttpResponseBodyStream.h"
#include "HttpResponseIterators.h"
#include "IHttpResponse.h"
#include <Arduino.h>
#include <memory>
namespace HttpServerAdvanced
{

  class HttpResponse : public IHttpResponse
  {
  private:
    HttpStatus status_;
    HttpHeaderCollection headers_;
    std::unique_ptr<HttpResponseBodyStream> body_;

  public:
    HttpResponse(HttpStatus status, std::unique_ptr<IByteSource> body, HttpHeaderCollection &&headers);
    HttpResponse(HttpStatus status, std::unique_ptr<Stream> body, HttpHeaderCollection &&headers);

    ~HttpResponse() override = default;

    HttpStatus status() const override;
    HttpHeaderCollection &headers() override;
    std::unique_ptr<HttpResponseBodyStream> getBody() override;
  };

  
  // Legacy Stream contract as currently used by the response pipeline:
  //  - available()  > 0 : that many bytes are readable now.
  //  - available() == 0 : terminal exhaustion for finite sources.
  //  - available()  < 0 : temporary-unavailable signaling used by adapters and
  //                       newer byte-source bridges.
  // read() and peek() return the next byte or -1 when no byte is produced for
  // the current call.
  std::unique_ptr<Stream> CreateResponseStream(std::unique_ptr<IHttpResponse> response);

} // namespace HttpServerAdvanced


