#pragma once
#include "../util/Util.h"
#include <http_parser.h>
namespace HttpServerAdvanced::Core
{
    using HttpServerAdvanced::StringView;
    class HttpMethod
    {
        // HTTP Methods are implemented as StringViews because the HTTP RFC explicity allows for
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

        static bool isRequestBodyAllowed(const StringView &method)
        {
            return method.equals(HttpMethod::Post) ||
                   method.equals(HttpMethod::Put) ||
                   method.equals(HttpMethod::Patch) ||
                   method.equals(HttpMethod::Delete);
        }

        template <typename... Args>
        static String anyOf(Args... args)
        {
            const HttpServerAdvanced::StringView methods[] = {args...};
            size_t total_length = 0;
            for (size_t i = 0; i < sizeof...(args); ++i)
            {
                total_length += methods[i].length();
            }
            String result;
            result.reserve(total_length + (sizeof...(args) - 1)); // for '|' separators
            for (size_t i = 0; i < sizeof...(args); ++i)
            {
                if (i > 0)
                {
                    result += "|";
                }
                result += String(methods[i].c_str(), methods[i].length());
            }
            return result;
        }
    };
} // namespace HttpServerAdvanced::Core