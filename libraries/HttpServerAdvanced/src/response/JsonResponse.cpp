#include "./JsonResponse.h"
#include "../core/HttpHeaderCollection.h"
#include "./HttpResponse.h"
#include "../streams/Streams.h"
#include "../core/HttpContentTypes.h"
namespace HttpServerAdvanced
{

    HttpHeaderCollection buildHeaders(std::initializer_list<HttpHeader> headers, size_t contentLength)
    {
        HttpHeaderCollection headersCollection;
        for (const auto &header : headers)
        {
            headersCollection.set(header);
        }
        if (!headersCollection.exists(HttpHeaderNames::ContentType))
        {
            headersCollection.set(HttpHeader(HttpHeaderNames::ContentType, HttpContentTypes::Json));
        }
        if (!headersCollection.exists(HttpHeaderNames::ContentLength))
        {
            headersCollection.set(HttpHeader(HttpHeaderNames::ContentLength, String(contentLength)));
        }
        return headersCollection;
    }

    std::unique_ptr<IHttpResponse> JsonResponse::create(HttpStatus status, const JsonDocument &doc, std::initializer_list<HttpHeader> headers)
    {
        // I'm not thrilled about this double serialization, but ArduinoJson doesn't provide a way to
        // serialize using a pull model.
        String body;
        body.reserve(measureJson(doc) + 1); // +1 for null terminator
        serializeJson(doc, body);
        auto headersCollection = buildHeaders(headers, body.length());
        auto bodyStream = std::make_unique<StringStream>(body);
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> JsonResponse::create(HttpStatus status, const JsonDocument &doc)
    {
        return create(status, doc, {});
    }


} // namespace HttpServerAdvanced


