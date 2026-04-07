#pragma once
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include "../core/HttpHeaderCollection.h"
#include "../response/HttpResponse.h"
#include "HandlerMatcher.h"
namespace httpadv::v1::routing
{
    using httpadv::v1::core::HttpHeaderNames;
    using httpadv::v1::core::HttpRequestContext;
    using httpadv::v1::response::HttpResponse;
    using httpadv::v1::response::IHttpResponse;

    struct CorsHandlingOptions
    {
        std::string allowedOrigins = "*";
        std::string allowedMethods = "*";
        std::string allowedHeaders = "*";
        std::string allowedCredentials;
        std::string exposeHeaders;
        int maxAge = -1;
        std::string requestHeaders;
        std::string requestMethods;

        CorsHandlingOptions() = default;

        CorsHandlingOptions(std::string_view allowedOriginsValue, std::string_view allowedMethodsValue, std::string_view allowedHeadersValue,
                            std::string_view allowedCredentialsValue = "", std::string_view exposeHeadersValue = "", int maxAgeValue = -1,
                            std::string_view requestHeadersValue = "", std::string_view requestMethodsValue = "")
            : allowedOrigins(allowedOriginsValue),
              allowedMethods(allowedMethodsValue),
              allowedHeaders(allowedHeadersValue),
              allowedCredentials(allowedCredentialsValue),
              exposeHeaders(exposeHeadersValue),
              maxAge(maxAgeValue),
              requestHeaders(requestHeadersValue),
              requestMethods(requestMethodsValue)
        {
        }
    };

    using CorsRequestFilter = std::function<bool(HttpRequestContext &)>;

    inline CorsHandlingOptions CreateCorsHandlingOptions(std::string_view allowedOrigins = "*", std::string_view allowedMethods = "*", std::string_view allowedHeaders = "*",
                                                         std::string_view allowedCredentials = "", std::string_view exposeHeaders = "", const int maxAge = -1,
                                                         std::string_view requestHeaders = "", std::string_view requestMethods = "")
    {
        return CorsHandlingOptions(allowedOrigins, allowedMethods, allowedHeaders,
                                   allowedCredentials, exposeHeaders, maxAge,
                                   requestHeaders, requestMethods);
    }

    inline CorsHandlingOptions CreateCorsHandlingOptions(const char *allowedOrigins, const char *allowedMethods, const char *allowedHeaders,
                                                         const char *allowedCredentials = "", const char *exposeHeaders = "", const int maxAge = -1,
                                                         const char *requestHeaders = "", const char *requestMethods = "")
    {
        return CreateCorsHandlingOptions(std::string_view(allowedOrigins != nullptr ? allowedOrigins : ""),
                                         std::string_view(allowedMethods != nullptr ? allowedMethods : ""),
                                         std::string_view(allowedHeaders != nullptr ? allowedHeaders : ""),
                                         std::string_view(allowedCredentials != nullptr ? allowedCredentials : ""),
                                         std::string_view(exposeHeaders != nullptr ? exposeHeaders : ""),
                                         maxAge,
                                         std::string_view(requestHeaders != nullptr ? requestHeaders : ""),
                                         std::string_view(requestMethods != nullptr ? requestMethods : ""));
    }

    inline HttpResponse::ResponseFilter CrossOriginRequestSharing(CorsHandlingOptions options)
    {
        return [options = std::move(options)](std::unique_ptr<IHttpResponse> resp) mutable -> std::unique_ptr<IHttpResponse>
        {
            if (!resp)
            {
                return resp;
            }
            auto& headers = resp->headers();
            if (!headers.exists(HttpHeaderNames::AccessControlAllowOrigin) && !options.allowedOrigins.empty())
                headers.set(HttpHeaderNames::AccessControlAllowOrigin, options.allowedOrigins.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlAllowMethods) && !options.allowedMethods.empty())
                headers.set(HttpHeaderNames::AccessControlAllowMethods, options.allowedMethods.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlAllowHeaders) && !options.allowedHeaders.empty())
                headers.set(HttpHeaderNames::AccessControlAllowHeaders, options.allowedHeaders.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlAllowCredentials) && !options.allowedCredentials.empty())
                headers.set(HttpHeaderNames::AccessControlAllowCredentials, options.allowedCredentials.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlExposeHeaders) && !options.exposeHeaders.empty())
                headers.set(HttpHeaderNames::AccessControlExposeHeaders, options.exposeHeaders.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlMaxAge) && options.maxAge >= 0)
                headers.set(std::string_view(HttpHeaderNames::AccessControlMaxAge), std::to_string(options.maxAge));
            if (!headers.exists(HttpHeaderNames::AccessControlRequestHeaders) && !options.requestHeaders.empty())
                headers.set(HttpHeaderNames::AccessControlRequestHeaders, options.requestHeaders.c_str());
            if (!headers.exists(HttpHeaderNames::AccessControlRequestMethod) && !options.requestMethods.empty())
                headers.set(HttpHeaderNames::AccessControlRequestMethod, options.requestMethods.c_str());
            return resp;
        };
    }

    inline HttpResponse::ResponseFilter CrossOriginRequestSharing(std::string_view allowedOrigins = "*", std::string_view allowedMethods = "*", std::string_view allowedHeaders = "*",
                                                                  std::string_view allowedCredentials = "", std::string_view exposeHeaders = "", const int maxAge = -1, std::string_view requestHeaders = "", std::string_view requestMethods = "")
    {
        return CrossOriginRequestSharing(CreateCorsHandlingOptions(allowedOrigins, allowedMethods, allowedHeaders,
                                                                   allowedCredentials, exposeHeaders, maxAge,
                                                                   requestHeaders, requestMethods));
    }

    inline HttpResponse::ResponseFilter CrossOriginRequestSharing(const char *allowedOrigins, const char *allowedMethods, const char *allowedHeaders,
                                                                  const char *allowedCredentials = "", const char *exposeHeaders = "", const int maxAge = -1,
                                                                  const char *requestHeaders = "", const char *requestMethods = "")
    {
        return CrossOriginRequestSharing(CreateCorsHandlingOptions(allowedOrigins, allowedMethods, allowedHeaders,
                                                                   allowedCredentials, exposeHeaders, maxAge,
                                                                   requestHeaders, requestMethods));
    }

    inline auto CreateCorsHandlingComponent(CorsRequestFilter requestFilter, CorsHandlingOptions options)
    {
        const auto filter = CrossOriginRequestSharing(std::move(options));

        return [requestFilter = std::move(requestFilter), filter](auto &builder)
        {
            builder.handlerProviders().with([requestFilter, filter](httpadv::v1::core::HttpRequestContext &context, httpadv::v1::handlers::IHttpHandler::InvocationNext next) -> httpadv::v1::handlers::IHttpHandler::HandlerResult
            {
                const bool shouldApply = !requestFilter || requestFilter(context);

                if (shouldApply && context.methodView() == "OPTIONS" && context.headers().exists(HttpHeaderNames::AccessControlRequestMethod))
                {
                    auto preflightResult = httpadv::v1::handlers::HandlerResult::responseResult(
                        std::make_unique<HttpResponse>(httpadv::v1::core::HttpStatus::NoContent(), nullptr, httpadv::v1::core::HttpHeaderCollection{}));
                    return httpadv::v1::handlers::HandlerResult::responseResult(filter(std::move(preflightResult.response)));
                }

                auto result = next ? next() : httpadv::v1::handlers::IHttpHandler::HandlerResult();

                if (shouldApply && result.isResponse())
                {
                    return httpadv::v1::handlers::HandlerResult::responseResult(filter(std::move(result.response)));
                }

                return result;
            });
        };
    }

    inline auto CorsHandling(std::string_view allowedOrigins = "*", std::string_view allowedMethods = "*", std::string_view allowedHeaders = "*",
                             std::string_view allowedCredentials = "", std::string_view exposeHeaders = "", const int maxAge = -1,
                             std::string_view requestHeaders = "", std::string_view requestMethods = "")
    {
        return CreateCorsHandlingComponent(CorsRequestFilter(),
                                           CreateCorsHandlingOptions(allowedOrigins, allowedMethods, allowedHeaders,
                                                                     allowedCredentials, exposeHeaders, maxAge,
                                                                     requestHeaders, requestMethods));
    }

    inline auto CorsHandling(CorsHandlingOptions options)
    {
        return CreateCorsHandlingComponent(CorsRequestFilter(), std::move(options));
    }

    inline auto CorsHandling(CorsRequestFilter requestFilter, CorsHandlingOptions options = {})
    {
        return CreateCorsHandlingComponent(std::move(requestFilter), std::move(options));
    }

    inline auto CorsHandling(std::string_view uriPattern, CorsHandlingOptions options)
    {
        const HandlerMatcher matcher(uriPattern);
        return CorsHandling(CorsRequestFilter([matcher](HttpRequestContext &context)
        {
            return matcher.canHandle(context);
        }), options);
    }

    inline auto CorsHandling(const char *uriPattern, CorsHandlingOptions options)
    {
        return CorsHandling(std::string_view(uriPattern != nullptr ? uriPattern : ""), options);
    }

    inline auto CorsHandling(const char *allowedOrigins, const char *allowedMethods, const char *allowedHeaders,
                             const char *allowedCredentials = "", const char *exposeHeaders = "", const int maxAge = -1,
                             const char *requestHeaders = "", const char *requestMethods = "")
    {
        return CorsHandling(CreateCorsHandlingOptions(allowedOrigins, allowedMethods, allowedHeaders,
                                                      allowedCredentials, exposeHeaders, maxAge,
                                                      requestHeaders, requestMethods));
    }

    

} // namespace HttpServerAdvanced


