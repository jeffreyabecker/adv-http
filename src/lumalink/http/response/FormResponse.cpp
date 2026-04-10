#include "FormResponse.h"

#include <string>
#include <utility>
#include <vector>

namespace lumalink::http::response
{
    using lumalink::platform::buffers::AvailableByteCount;
    using lumalink::platform::buffers::HasAvailableBytes;

    namespace
    {
        HttpHeaderCollection buildFormHeaders(std::initializer_list<HttpHeader> headers, size_t contentLength)
        {
            HttpHeaderCollection headersCollection;
            for (const auto &header : headers)
            {
                headersCollection.set(header);
            }
            if (!headersCollection.exists(HttpHeaderNames::ContentType))
            {
                headersCollection.set(HttpHeader(HttpHeaderNames::ContentType, "application/x-www-form-urlencoded"));
            }
            if (!headersCollection.exists(HttpHeaderNames::ContentLength))
            {
                headersCollection.set(HttpHeader(std::string_view(HttpHeaderNames::ContentLength), std::to_string(contentLength)));
            }
            return headersCollection;
        }
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        FieldCollection &&data,
        std::initializer_list<HttpHeader> headers)
    {
        auto formStream = std::make_unique<lumalink::http::streams::FormEncodingStream>(std::move(data));
        const auto available = formStream->available();
        size_t contentLength = HasAvailableBytes(available) ? AvailableByteCount(available) : 0;
        auto headersCollection = buildFormHeaders(headers, contentLength);
        return std::make_unique<HttpResponse>(status, std::move(formStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        const FieldCollection &data,
        std::initializer_list<HttpHeader> headers)
    {
        // Copy the data and delegate to the rvalue reference version
        auto dataCopy = data;
        return create(status, std::move(dataCopy), headers);
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        FieldMap &&data,
        std::initializer_list<HttpHeader> headers)
    {
        // Convert map to vector of pairs and delegate
        FieldCollection vectorData;
        vectorData.reserve(data.size());
        for (auto &pair : data)
        {
            vectorData.emplace_back(pair.first, std::move(pair.second));
        }
        return create(status, std::move(vectorData), headers);
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        const FieldMap &data,
        std::initializer_list<HttpHeader> headers)
    {
        // Convert map to vector of pairs and delegate
        FieldCollection vectorData;
        vectorData.reserve(data.size());
        for (const auto &pair : data)
        {
            vectorData.emplace_back(pair);
        }
        return create(status, std::move(vectorData), headers);
    }

} // namespace lumalink::http::response
