#pragma once
#include "../util/Util.h"
namespace HttpServerAdvanced::Core
{
    using HttpServerAdvanced::StringView;
    class HttpMethod
    {
        //HTTP Methods are implemented as StringViews because the HTTP RFC explicity allows for
        //non-standard methods, so we can't use an enum here.
        //
    public:
        static const HttpServerAdvanced::StringView DELETE;
        static const HttpServerAdvanced::StringView GET;
        static const HttpServerAdvanced::StringView HEAD;
        static const HttpServerAdvanced::StringView POST;
        static const HttpServerAdvanced::StringView PUT;
        static const HttpServerAdvanced::StringView CONNECT;
        static const HttpServerAdvanced::StringView OPTIONS;
        static const HttpServerAdvanced::StringView TRACE;
        static const HttpServerAdvanced::StringView PATCH;
        static const HttpServerAdvanced::StringView PURGE;
        static const HttpServerAdvanced::StringView LINK;
        static const HttpServerAdvanced::StringView UNLINK;

        static bool isRequestBodyAllowed(const StringView &method)
        {
            return method.equals(HttpMethod::POST) ||
                   method.equals(HttpMethod::PUT) ||
                   method.equals(HttpMethod::PATCH) ||
                   method.equals(HttpMethod::DELETE);
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
            result.resverve(total_length + (sizeof...(args) - 1)); // for '|' separators
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