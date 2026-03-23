#include "../core/Defines.h"   
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
#include "JsonResponse.h"
#include "../core/HttpHeaderCollection.h"
#include "HttpResponse.h"
#include "../streams/ByteStream.h"
#include "../core/HttpContentTypes.h"

namespace HttpServerAdvanced
{

    namespace
    {
        HttpHeaderCollection buildJsonHeaders(std::initializer_list<HttpHeader> headers, size_t contentLength)
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
                headersCollection.set(HttpHeader(std::string_view(HttpHeaderNames::ContentLength), std::to_string(contentLength)));
            }
            return headersCollection;
        }
    }

    std::unique_ptr<IHttpResponse> JsonResponse::create(HttpStatus status, const JsonDocument &doc, std::initializer_list<HttpHeader> headers)
    {
        // I'm not thrilled about this double serialization, but ArduinoJson doesn't provide a way to
        // serialize using a pull model.
        std::string body;
        body.reserve(measureJson(doc));
        serializeJson(doc, body);
        auto headersCollection = buildJsonHeaders(headers, body.size());
        auto bodySource = std::make_unique<StdStringByteSource>(std::move(body));
        return std::make_unique<HttpResponse>(status, std::move(bodySource), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> JsonResponse::create(HttpStatus status, const JsonDocument &doc)
    {
        return create(status, doc, {});
    }


} // namespace HttpServerAdvanced
#endif

