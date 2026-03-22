#include "FormResponse.h"

#include <Arduino.h>

#include <string>
#include <utility>
#include <vector>

namespace HttpServerAdvanced
{
    namespace
    {
        bool isUnreservedFormChar(unsigned char value)
        {
            return (value >= 'A' && value <= 'Z') ||
                   (value >= 'a' && value <= 'z') ||
                   (value >= '0' && value <= '9') ||
                   value == '-' || value == '_' || value == '.' || value == '*';
        }

        std::string encodeFormComponent(std::string_view value)
        {
            static constexpr char Hex[] = "0123456789ABCDEF";

            std::string encoded;
            encoded.reserve(value.size());
            for (unsigned char ch : value)
            {
                if (isUnreservedFormChar(ch))
                {
                    encoded.push_back(static_cast<char>(ch));
                }
                else if (ch == ' ')
                {
                    encoded.push_back('+');
                }
                else
                {
                    encoded.push_back('%');
                    encoded.push_back(Hex[(ch >> 4) & 0x0F]);
                    encoded.push_back(Hex[ch & 0x0F]);
                }
            }

            return encoded;
        }

        std::string buildFormBody(const FormResponse::FieldCollection &data)
        {
            std::string body;
            bool first = true;
            for (const auto &pair : data)
            {
                if (!first)
                {
                    body.push_back('&');
                }
                first = false;

                body.append(encodeFormComponent(pair.first));
                if (!pair.second.empty())
                {
                    body.push_back('=');
                    body.append(encodeFormComponent(pair.second));
                }
            }

            return body;
        }

        FormResponse::FieldCollection toOwnedFields(const std::vector<std::pair<String, String>> &data)
        {
            FormResponse::FieldCollection owned;
            owned.reserve(data.size());
            for (const auto &pair : data)
            {
                owned.emplace_back(
                    std::string(pair.first.c_str(), pair.first.length()),
                    std::string(pair.second.c_str(), pair.second.length()));
            }
            return owned;
        }

        FormResponse::FieldCollection toOwnedFields(const std::map<String, String> &data)
        {
            FormResponse::FieldCollection owned;
            owned.reserve(data.size());
            for (const auto &pair : data)
            {
                owned.emplace_back(
                    std::string(pair.first.c_str(), pair.first.length()),
                    std::string(pair.second.c_str(), pair.second.length()));
            }
            return owned;
        }

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
        std::string body = buildFormBody(data);
        size_t contentLength = body.size();
        auto headersCollection = buildFormHeaders(headers, contentLength);
        return std::make_unique<HttpResponse>(status, std::make_unique<StdStringByteSource>(std::move(body)), std::move(headersCollection));
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

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        std::vector<std::pair<String, String>> &&data,
        std::initializer_list<HttpHeader> headers)
    {
        return create(status, toOwnedFields(data), headers);
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        const std::vector<std::pair<String, String>> &data,
        std::initializer_list<HttpHeader> headers)
    {
        return create(status, toOwnedFields(data), headers);
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        std::map<String, String> &&data,
        std::initializer_list<HttpHeader> headers)
    {
        return create(status, toOwnedFields(data), headers);
    }

    std::unique_ptr<IHttpResponse> FormResponse::create(
        HttpStatus status,
        const std::map<String, String> &data,
        std::initializer_list<HttpHeader> headers)
    {
        return create(status, toOwnedFields(data), headers);
    }

} // namespace HttpServerAdvanced
