#include "./FormResponse.h"

namespace HttpServerAdvanced
{
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
                headersCollection.set(HttpHeader(HttpHeaderNames::ContentLength, String(contentLength)));
            }
            return headersCollection;
        }
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        std::vector<std::pair<String, String>> &&data,
        std::initializer_list<HttpHeader> headers)
    {
        // Create the form encoding stream; it will calculate total length internally
        auto formStream = std::make_unique<FormEncodingStream>(std::move(data));
        size_t contentLength = formStream->available();
        auto headersCollection = buildFormHeaders(headers, contentLength);
        return std::make_unique<HttpResponse>(status, std::move(formStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        const std::vector<std::pair<String, String>> &data,
        std::initializer_list<HttpHeader> headers)
    {
        // Copy the data and delegate to the rvalue reference version
        auto dataCopy = data;
        return create(status, std::move(dataCopy), headers);
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        std::map<String, String> &&data,
        std::initializer_list<HttpHeader> headers)
    {
        // Convert map to vector of pairs and delegate
        std::vector<std::pair<String, String>> vectorData;
        for (auto &pair : data)
        {
            vectorData.emplace_back(std::move(pair));
        }
        return create(status, std::move(vectorData), headers);
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        const std::map<String, String> &data,
        std::initializer_list<HttpHeader> headers)
    {
        // Convert map to vector of pairs and delegate
        std::vector<std::pair<String, String>> vectorData;
        for (const auto &pair : data)
        {
            vectorData.emplace_back(pair);
        }
        return create(status, std::move(vectorData), headers);
    }

} // namespace HttpServerAdvanced
