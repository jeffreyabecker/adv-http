#include "./HttpMethod.h"

namespace HttpServerAdvanced::Core
{
    const HttpServerAdvanced::StringView HttpMethod::DELETE("DELETE");
    const HttpServerAdvanced::StringView HttpMethod::GET("GET");
    const HttpServerAdvanced::StringView HttpMethod::HEAD("HEAD");
    const HttpServerAdvanced::StringView HttpMethod::POST("POST");
    const HttpServerAdvanced::StringView HttpMethod::PUT("PUT");
    const HttpServerAdvanced::StringView HttpMethod::CONNECT("CONNECT");
    const HttpServerAdvanced::StringView HttpMethod::OPTIONS("OPTIONS");
    const HttpServerAdvanced::StringView HttpMethod::TRACE("TRACE");
    const HttpServerAdvanced::StringView HttpMethod::PATCH("PATCH");
    const HttpServerAdvanced::StringView HttpMethod::PURGE("PURGE");
    const HttpServerAdvanced::StringView HttpMethod::LINK("LINK");
    const HttpServerAdvanced::StringView HttpMethod::UNLINK("UNLINK");


    bool HttpMethod::isRequestBodyAllowed(const StringView &method)
    {
        return method.equals(HttpMethod::POST) ||
               method.equals(HttpMethod::PUT) ||
               method.equals(HttpMethod::PATCH) ||
               method.equals(HttpMethod::DELETE);
    }
} // namespace HttpServerAdvanced::Core