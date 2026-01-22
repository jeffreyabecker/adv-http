#pragma once
#include <Arduino.h>
#include <memory>
#include <functional>
#include "./HttpResponse.h"
namespace HttpServerAdvanced
{

    HttpResponse::ResponseFilter CrossOriginRequestSharing(const String &allowedOrigins = "*", const String &allowedMethods = "*", const String &allowedHeaders = "*",
                                                           const String &allowedCredentials = "", const String &exposeHeaders = "", const int maxAge = -1, const String &requestHeaders = "", const String &requestMethods = "")
    {
        return [=](std::unique_ptr<IHttpResponse> resp) -> std::unique_ptr<IHttpResponse>
        {
            if (!resp)
            {
                return resp;
            }
            if (!resp->headers().exists(HttpHeaderNames::AccessControlAllowOrigin) && !allowedOrigins.isEmpty())
            {
                resp->headers().set(HttpHeaderNames::AccessControlAllowOrigin, allowedOrigins);
            }
            if (!resp->headers().exists(HttpHeaderNames::AccessControlAllowMethods) && !allowedMethods.isEmpty())
            {
                resp->headers().set(HttpHeaderNames::AccessControlAllowMethods, allowedMethods);
            }
            if (!resp->headers().exists(HttpHeaderNames::AccessControlAllowHeaders) && !allowedHeaders.isEmpty())
            {
                resp->headers().set(HttpHeaderNames::AccessControlAllowHeaders, allowedHeaders);
            }
            if (!resp->headers().exists(HttpHeaderNames::AccessControlAllowCredentials) && !allowedCredentials.isEmpty())
            {
                resp->headers().set(HttpHeaderNames::AccessControlAllowCredentials, allowedCredentials);
            }
            if (!resp->headers().exists(HttpHeaderNames::AccessControlExposeHeaders) && !exposeHeaders.isEmpty())
            {
                resp->headers().set(HttpHeaderNames::AccessControlExposeHeaders, exposeHeaders);
            }
            if (!resp->headers().exists(HttpHeaderNames::AccessControlMaxAge) && maxAge >= 0)
            {
                resp->headers().set(HttpHeaderNames::AccessControlMaxAge, String(maxAge));
            }
            if (!resp->headers().exists(HttpHeaderNames::AccessControlRequestHeaders) && !requestHeaders.isEmpty())
            {
                resp->headers().set(HttpHeaderNames::AccessControlRequestHeaders, requestHeaders);
            }
            if (!resp->headers().exists(HttpHeaderNames::AccessControlRequestMethod) && !requestMethods.isEmpty())
            {
                resp->headers().set(HttpHeaderNames::AccessControlRequestMethod, requestMethods);
            }
            return resp;
        };
    }

} // namespace HttpServerAdvanced