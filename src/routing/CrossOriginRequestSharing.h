#pragma once
#include <Arduino.h>
#include <memory>
#include <functional>
#include "../response/HttpResponse.h"
namespace HttpServerAdvanced
{

    inline HttpResponse::ResponseFilter CrossOriginRequestSharing(const String &allowedOrigins = "*", const String &allowedMethods = "*", const String &allowedHeaders = "*",
                                                                  const String &allowedCredentials = "", const String &exposeHeaders = "", const int maxAge = -1, const String &requestHeaders = "", const String &requestMethods = "")
    {
        return [=](std::unique_ptr<IHttpResponse> resp) -> std::unique_ptr<IHttpResponse>
        {
            if (!resp)
            {
                return resp;
            }
            auto& headers = resp->headers();
            if (!headers.exists(HttpHeaderNames::AccessControlAllowOrigin) && !allowedOrigins.isEmpty())
                headers.set(HttpHeaderNames::AccessControlAllowOrigin, allowedOrigins);
            if (!headers.exists(HttpHeaderNames::AccessControlAllowMethods) && !allowedMethods.isEmpty())
                headers.set(HttpHeaderNames::AccessControlAllowMethods, allowedMethods);
            if (!headers.exists(HttpHeaderNames::AccessControlAllowHeaders) && !allowedHeaders.isEmpty())
                headers.set(HttpHeaderNames::AccessControlAllowHeaders, allowedHeaders);
            if (!headers.exists(HttpHeaderNames::AccessControlAllowCredentials) && !allowedCredentials.isEmpty())
                headers.set(HttpHeaderNames::AccessControlAllowCredentials, allowedCredentials);
            if (!headers.exists(HttpHeaderNames::AccessControlExposeHeaders) && !exposeHeaders.isEmpty())
                headers.set(HttpHeaderNames::AccessControlExposeHeaders, exposeHeaders);
            if (!headers.exists(HttpHeaderNames::AccessControlMaxAge) && maxAge >= 0)
                headers.set(HttpHeaderNames::AccessControlMaxAge, String(maxAge));
            if (!headers.exists(HttpHeaderNames::AccessControlRequestHeaders) && !requestHeaders.isEmpty())
                headers.set(HttpHeaderNames::AccessControlRequestHeaders, requestHeaders);
            if (!headers.exists(HttpHeaderNames::AccessControlRequestMethod) && !requestMethods.isEmpty())
                headers.set(HttpHeaderNames::AccessControlRequestMethod, requestMethods);
            return resp;
        };
    }

    inline HttpResponse::ResponseFilter CrossOriginRequestSharing(const char *allowedOrigins, const char *allowedMethods, const char *allowedHeaders,
                                                                  const char *allowedCredentials = "", const char *exposeHeaders = "", const int maxAge = -1,
                                                                  const char *requestHeaders = "", const char *requestMethods = "")
    {
        return CrossOriginRequestSharing(String(allowedOrigins != nullptr ? allowedOrigins : ""),
                                         String(allowedMethods != nullptr ? allowedMethods : ""),
                                         String(allowedHeaders != nullptr ? allowedHeaders : ""),
                                         String(allowedCredentials != nullptr ? allowedCredentials : ""),
                                         String(exposeHeaders != nullptr ? exposeHeaders : ""),
                                         maxAge,
                                         String(requestHeaders != nullptr ? requestHeaders : ""),
                                         String(requestMethods != nullptr ? requestMethods : ""));
    }

} // namespace HttpServerAdvanced


