#include "HttpResponse.h"
#include "./Streams.h"
#include "./core/HttpContentTypes.h"
namespace HttpServerAdvanced
{



    HttpResponse::HttpResponse(HttpStatus status, std::unique_ptr<Stream> body, HttpHeaderCollection &&headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body)))
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

    std::unique_ptr<HttpResponseBodyStream> HttpResponse::getBody()
    {
        // Extract the body stream for use in response writing.
        // The body is moved only once to the HttpPipelineResponseStream.
        return std::move(body_);
    }

} // namespace HttpServerAdvanced
