#pragma once

#include "../core/Defines.h"
#include "../core/HttpContext.h"
#include "../response/HttpResponse.h"
#include "../core/HttpStatus.h"
#include "../util/HttpUtility.h"
#include "../handlers/HttpHandler.h"
#include "../response/StringResponse.h"
#include <memory>
#include <string>
#include <string_view>

namespace httpadv::v1::routing
{
    using httpadv::v1::core::HttpContext;
    using httpadv::v1::core::HttpHeader;
    using httpadv::v1::core::HttpHeaderNames;
    using httpadv::v1::core::HttpStatus;
    using httpadv::v1::handlers::IHttpHandler;
    using httpadv::v1::response::IHttpResponse;
    using httpadv::v1::response::StringResponse;

    namespace BasicAuthImpl
    {
        inline bool CheckBasicAuthCredentials(httpadv::v1::core::HttpContext &context, std::function<bool(std::string_view, std::string_view)> validator,
                              std::function<void(std::string_view, std::string_view)> onSuccess)
        {
            auto authHeaderOpt = context.headers().find(HttpHeaderNames::Authorization);
            if (!authHeaderOpt.has_value())
            {
                return false;
            }
            auto authHeader = authHeaderOpt.value();
            static constexpr std::string_view Prefix("Basic ");
            const std::string_view authHeaderValue = authHeader.valueView();
            if (authHeaderValue.size() < Prefix.size() || authHeaderValue.substr(0, Prefix.size()) != Prefix)
            {
                return false;
            }
            const std::string decodedCredentials = httpadv::v1::util::WebUtility::Base64DecodeToString(authHeaderValue.substr(Prefix.size()));
            const std::string_view decodedView(decodedCredentials.data(), decodedCredentials.size());
            const std::size_t separatorIndex = decodedView.find(':');
            if (separatorIndex == std::string_view::npos)
            {
                return false;
            }
            const std::string_view username = decodedView.substr(0, separatorIndex);
            const std::string_view password = decodedView.substr(separatorIndex + 1);
            if (!validator(username, password))
            {
                return false;
            }
            if (onSuccess)
            {
                onSuccess(username, password);
            }
            else
            {
                context.items().emplace("BasicAuth::Username", std::string(username));
                context.items().emplace("BasicAuth::Password", std::string(password));
            }
            return true;
        }
    }

    inline std::unique_ptr<IHttpResponse> defaultOnFailure(httpadv::v1::core::HttpContext &context, std::string_view realm)
    {
        return StringResponse::create(
            HttpStatus::Unauthorized(),
            std::string_view("Unauthorized"),
            {HttpHeader(std::string_view(HttpHeaderNames::WwwAuthenticate), std::string("Basic realm=\"") + std::string(realm) + "\"")});
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(std::function<bool(std::string_view, std::string_view)> validator, std::string_view realm = "Restricted Area",
                                                       std::function<void(std::string_view, std::string_view)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(httpadv::v1::core::HttpContext &context, std::string_view)> onFailure = defaultOnFailure)
    {
        std::string realmValue(realm);

        return [validator = std::move(validator), realmValue = std::move(realmValue), onSuccess = std::move(onSuccess), onFailure = std::move(onFailure)](httpadv::v1::core::HttpContext &context, IHttpHandler::InvocationCallback next) -> IHttpHandler::HandlerResult
        {
            if (BasicAuthImpl::CheckBasicAuthCredentials(context, validator, onSuccess))
            {
                return next(context);
            }
            return IHttpHandler::HandlerResult::responseResult(onFailure(context, realmValue));
        };
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(std::string_view expectedUsername, std::string_view expectedPassword, std::string_view realm = "Restricted Area",
                                                       std::function<void(std::string_view, std::string_view)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(httpadv::v1::core::HttpContext &context, std::string_view)> onFailure = defaultOnFailure)
    {
        return BasicAuth(
            [expectedUsername, expectedPassword](std::string_view foundUsername, std::string_view foundPassword)
            {
                return foundUsername == expectedUsername && foundPassword == expectedPassword;
            },
            realm,
            std::move(onSuccess),
            std::move(onFailure));
    }
} // namespace HttpServerAdvanced





