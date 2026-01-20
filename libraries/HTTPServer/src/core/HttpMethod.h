#pragma once
#include "../util/Util.h"
namespace HTTPServer::Core
{
    using HTTPServer::StringView;
    class HttpMethod
    {
        //HTTP Methods are implemented as StringViews because the HTTP RFC explicity allows for
        //non-standard methods, so we can't use an enum here.
        //
    public:
        static const HTTPServer::StringView DELETE;
        static const HTTPServer::StringView GET;
        static const HTTPServer::StringView HEAD;
        static const HTTPServer::StringView POST;
        static const HTTPServer::StringView PUT;
        static const HTTPServer::StringView CONNECT;
        static const HTTPServer::StringView OPTIONS;
        static const HTTPServer::StringView TRACE;
        static const HTTPServer::StringView PATCH;
        static const HTTPServer::StringView PURGE;
        static const HTTPServer::StringView LINK;
        static const HTTPServer::StringView UNLINK;

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
            const HTTPServer::StringView methods[] = {args...};
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
} // namespace HTTPServer::Core