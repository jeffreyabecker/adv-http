#pragma once
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include "../response/HttpResponse.h"
namespace HttpServerAdvanced
{

    inline HttpResponse::ResponseFilter CrossOriginRequestSharing(std::string_view allowedOrigins = "*", std::string_view allowedMethods = "*", std::string_view allowedHeaders = "*",
                                                                  std::string_view allowedCredentials = "", std::string_view exposeHeaders = "", const int maxAge = -1, std::string_view requestHeaders = "", std::string_view requestMethods = "")
    {
        const std::string allowedOriginsValue(allowedOrigins);
        const std::string allowedMethodsValue(allowedMethods);
        const std::string allowedHeadersValue(allowedHeaders);
        const std::string allowedCredentialsValue(allowedCredentials);
        const std::string exposeHeadersValue(exposeHeaders);
        const std::string requestHeadersValue(requestHeaders);
        const std::string requestMethodsValue(requestMethods);

        return [allowedOriginsValue, allowedMethodsValue, allowedHeadersValue, allowedCredentialsValue, exposeHeadersValue, maxAge, requestHeadersValue, requestMethodsValue](std::unique_ptr<IHttpResponse> resp) -> std::unique_ptr<IHttpResponse>
        {
            if (!resp)
            {
                return resp;
            }
            auto& headers = resp->headers();
            if (!headers.exists(HttpHeaderNames::AccessControlAllowOrigin) && !allowedOriginsValue.empty())
                headers.set(HttpHeaderNames::AccessControlAllowOrigin, allowedOriginsValue.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlAllowMethods) && !allowedMethodsValue.empty())
                headers.set(HttpHeaderNames::AccessControlAllowMethods, allowedMethodsValue.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlAllowHeaders) && !allowedHeadersValue.empty())
                headers.set(HttpHeaderNames::AccessControlAllowHeaders, allowedHeadersValue.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlAllowCredentials) && !allowedCredentialsValue.empty())
                headers.set(HttpHeaderNames::AccessControlAllowCredentials, allowedCredentialsValue.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlExposeHeaders) && !exposeHeadersValue.empty())
                headers.set(HttpHeaderNames::AccessControlExposeHeaders, exposeHeadersValue.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlMaxAge) && maxAge >= 0)
                headers.set(std::string_view(HttpHeaderNames::AccessControlMaxAge), std::to_string(maxAge));
            if (!headers.exists(HttpHeaderNames::AccessControlRequestHeaders) && !requestHeadersValue.empty())
                headers.set(HttpHeaderNames::AccessControlRequestHeaders, requestHeadersValue.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlRequestMethod) && !requestMethodsValue.empty())
                headers.set(HttpHeaderNames::AccessControlRequestMethod, requestMethodsValue.c_str());
            return resp;
        };
    }

    inline HttpResponse::ResponseFilter CrossOriginRequestSharing(const char *allowedOrigins, const char *allowedMethods, const char *allowedHeaders,
                                                                  const char *allowedCredentials = "", const char *exposeHeaders = "", const int maxAge = -1,
                                                                  const char *requestHeaders = "", const char *requestMethods = "")
    {
        return CrossOriginRequestSharing(std::string_view(allowedOrigins != nullptr ? allowedOrigins : ""),
                                         std::string_view(allowedMethods != nullptr ? allowedMethods : ""),
                                         std::string_view(allowedHeaders != nullptr ? allowedHeaders : ""),
                                         std::string_view(allowedCredentials != nullptr ? allowedCredentials : ""),
                                         std::string_view(exposeHeaders != nullptr ? exposeHeaders : ""),
                                         maxAge,
                                         std::string_view(requestHeaders != nullptr ? requestHeaders : ""),
                                         std::string_view(requestMethods != nullptr ? requestMethods : ""));
    }

} // namespace HttpServerAdvanced


