#include "HttpResponse.h"
#include "../streams/Streams.h"
#include "../core/HttpContentTypes.h"
namespace httpadv::v1::response
{

    HttpResponse::HttpResponse(HttpStatus status, std::unique_ptr<IByteSource> body, HttpHeaderCollection &&headers)
        : status_(status), headers_(std::move(headers)), body_(std::move(body))
    {
    }

    HttpStatus HttpResponse::status() const
    {
        return status_;
    }

    HttpHeaderCollection &HttpResponse::headers()
    {
        return headers_;
    }

    std::unique_ptr<IByteSource> HttpResponse::getBody()
    {
        return std::move(body_);
    }

} // namespace HttpServerAdvanced


