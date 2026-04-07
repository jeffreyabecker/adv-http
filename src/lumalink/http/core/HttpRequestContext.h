#pragma once

#include "HttpStatus.h"
#include "HttpContextPhase.h"

#include <any>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace lumalink::http::core
{
    class HttpHeaderCollection;
}

namespace lumalink::http::util
{
    class UriView;
}

namespace lumalink::http::core
{
    class HttpRequestContext
    {
    private:
        mutable std::map<std::string, std::any> items_;

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
        virtual HttpContextPhaseFlags completedPhases() const = 0;
        std::map<std::string, std::any> &items() const { return items_; }
        virtual lumalink::http::util::UriView &uriView() const = 0;
    };
}