#pragma once

#include "HttpStatus.h"

#include <any>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace httpadv::v1::core
{
    class HttpHeaderCollection;
}

namespace httpadv::v1::response
{
    class IHttpResponse;
}

namespace httpadv::v1::util
{
    class UriView;
}

namespace httpadv::v1::core
{
    class HttpRequestContext
    {
    public:
        virtual ~HttpRequestContext() = default;

        virtual std::string_view version() const = 0;
        virtual std::string_view versionView() const = 0;
        virtual const char *method() const = 0;
        virtual std::string_view methodView() const = 0;
        virtual std::string_view url() const = 0;
        virtual std::string_view urlView() const = 0;
        virtual const HttpHeaderCollection &headers() const = 0;
        virtual std::string_view remoteAddress() const = 0;
        virtual uint16_t remotePort() const = 0;
        virtual std::string_view localAddress() const = 0;
        virtual uint16_t localPort() const = 0;
        virtual std::map<std::string, std::any> &items() const = 0;
        virtual httpadv::v1::util::UriView &uriView() const = 0;
        virtual std::unique_ptr<httpadv::v1::response::IHttpResponse> createResponse(HttpStatus status, std::string body) = 0;

        std::unique_ptr<httpadv::v1::response::IHttpResponse> createResponse(HttpStatus status, std::string_view body)
        {
            return createResponse(status, std::string(body));
        }

        std::unique_ptr<httpadv::v1::response::IHttpResponse> createResponse(HttpStatus status, const char *body)
        {
            return createResponse(status, std::string(body != nullptr ? body : ""));
        }
    };
}