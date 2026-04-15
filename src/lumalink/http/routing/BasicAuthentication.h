#pragma once

#include "../core/Defines.h"
#include "../core/HttpHeader.h"
#include "../core/HttpRequestContext.h"
#include "../response/HttpResponse.h"
#include "../core/HttpStatus.h"
#include "../util/HttpUtility.h"
#include "../handlers/IHttpHandler.h"
#include "../response/StringResponse.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace lumalink::http::routing
{
    using lumalink::http::core::HttpHeader;
    using lumalink::http::core::HttpHeaderNames;
    using lumalink::http::core::HttpRequestContext;
    using lumalink::http::core::HttpStatus;
    using lumalink::http::handlers::IHttpHandler;
    using lumalink::http::response::IHttpResponse;
    using lumalink::http::response::StringResponse;

    namespace BasicAuthImpl
    {
        bool CheckBasicAuthCredentials(HttpRequestContext &context, std::function<bool(std::string_view, std::string_view)> validator,
                                       std::function<void(std::string_view, std::string_view)> onSuccess);
    }

    std::unique_ptr<IHttpResponse> defaultOnFailure(HttpRequestContext &context, std::string_view realm);

    IHttpHandler::InterceptorCallback BasicAuth(std::function<bool(std::string_view, std::string_view)> validator, std::string_view realm = "Restricted Area",
                                                std::function<void(std::string_view, std::string_view)> onSuccess = nullptr,
                                                std::function<std::unique_ptr<IHttpResponse>(HttpRequestContext &context, std::string_view)> onFailure = defaultOnFailure);

    IHttpHandler::InterceptorCallback BasicAuth(std::string_view expectedUsername, std::string_view expectedPassword, std::string_view realm = "Restricted Area",
                                                std::function<void(std::string_view, std::string_view)> onSuccess = nullptr,
                                                std::function<std::unique_ptr<IHttpResponse>(HttpRequestContext &context, std::string_view)> onFailure = defaultOnFailure);
} // namespace lumalink::http::routing





