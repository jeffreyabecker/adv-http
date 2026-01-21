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
            if (!resp->headers().exists(HttpHeader::AccessControlAllowOrigin) && !allowedOrigins.isEmpty())
            {
                resp->headers().set(HttpHeader::AccessControlAllowOrigin, allowedOrigins);
            }
            if (!resp->headers().exists(HttpHeader::AccessControlAllowMethods) && !allowedMethods.isEmpty())
            {
                resp->headers().set(HttpHeader::AccessControlAllowMethods, allowedMethods);
            }
            if (!resp->headers().exists(HttpHeader::AccessControlAllowHeaders) && !allowedHeaders.isEmpty())
            {
                resp->headers().set(HttpHeader::AccessControlAllowHeaders, allowedHeaders);
            }
            if (!resp->headers().exists(HttpHeader::AccessControlAllowCredentials) && !allowedCredentials.isEmpty())
            {
                resp->headers().set(HttpHeader::AccessControlAllowCredentials, allowedCredentials);
            }
            if (!resp->headers().exists(HttpHeader::AccessControlExposeHeaders) && !exposeHeaders.isEmpty())
            {
                resp->headers().set(HttpHeader::AccessControlExposeHeaders, exposeHeaders);
            }
            if (!resp->headers().exists(HttpHeader::AccessControlMaxAge) && maxAge >= 0)
            {
                resp->headers().set(HttpHeader::AccessControlMaxAge, String(maxAge));
            }
            if (!resp->headers().exists(HttpHeader::AccessControlRequestHeaders) && !requestHeaders.isEmpty())
            {
                resp->headers().set(HttpHeader::AccessControlRequestHeaders, requestHeaders);
            }
            if (!resp->headers().exists(HttpHeader::AccessControlRequestMethod) && !requestMethods.isEmpty())
            {
                resp->headers().set(HttpHeader::AccessControlRequestMethod, requestMethods);
            }
            return resp;
        };
    }

} // namespace HttpServerAdvanced