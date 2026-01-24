#pragma once

#include <http_parser.h>
#include <cstring>
namespace HttpServerAdvanced
{

    class HttpMethod
    {
        // HTTP Methods are implemented as Strings because the HTTP RFC explicity allows for
        // non-standard methods, so we can't use an enum here.
        //
    public:
        static constexpr const char *Get = "GET";
        static constexpr const char *Head = "HEAD";
        static constexpr const char *Post = "POST";
        static constexpr const char *Put = "PUT";
        static constexpr const char *Connect = "CONNECT";
        static constexpr const char *Options = "OPTIONS";
        static constexpr const char *Trace = "TRACE";
        static constexpr const char *Patch = "PATCH";
        static constexpr const char *Purge = "PURGE";
        static constexpr const char *Link = "LINK";
        static constexpr const char *Unlink = "UNLINK";
        static constexpr const char *Delete = "DELETE";

        static bool isRequestBodyAllowed(const char *method)
        {
            return strcmp(method, HttpMethod::Post) == 0 ||
                   strcmp(method, HttpMethod::Put) == 0 ||
                   strcmp(method, HttpMethod::Patch) == 0 ||
                   strcmp(method, HttpMethod::Delete) == 0;
        }
    };
} // namespace HttpServerAdvanced