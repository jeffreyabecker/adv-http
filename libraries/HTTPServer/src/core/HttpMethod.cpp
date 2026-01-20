#include "./HttpMethod.h"

namespace HTTPServer::Core
{
    const HTTPServer::StringView HttpMethod::DELETE("DELETE");
    const HTTPServer::StringView HttpMethod::GET("GET");
    const HTTPServer::StringView HttpMethod::HEAD("HEAD");
    const HTTPServer::StringView HttpMethod::POST("POST");
    const HTTPServer::StringView HttpMethod::PUT("PUT");
    const HTTPServer::StringView HttpMethod::CONNECT("CONNECT");
    const HTTPServer::StringView HttpMethod::OPTIONS("OPTIONS");
    const HTTPServer::StringView HttpMethod::TRACE("TRACE");
    const HTTPServer::StringView HttpMethod::PATCH("PATCH");
    const HTTPServer::StringView HttpMethod::PURGE("PURGE");
    const HTTPServer::StringView HttpMethod::LINK("LINK");
    const HTTPServer::StringView HttpMethod::UNLINK("UNLINK");


    bool HttpMethod::isRequestBodyAllowed(const StringView &method)
    {
        return method.equals(HttpMethod::POST) ||
               method.equals(HttpMethod::PUT) ||
               method.equals(HttpMethod::PATCH) ||
               method.equals(HttpMethod::DELETE);
    }
} // namespace HTTPServer::Core