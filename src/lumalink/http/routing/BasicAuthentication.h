#pragma once

#include "../core/Defines.h"
#include "../core/HttpHeader.h"
#include "../core/HttpRequestContext.h"
#include "../response/HttpResponse.h"
#include "../core/HttpStatus.h"
#include "../util/HttpUtility.h"
#include "../handlers/IHttpHandler.h"
#include "../response/StringResponse.h"
#include <concepts>
#include <functional>
#include <utility>
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

    using BasicAuthFailureCallback = std::function<std::unique_ptr<IHttpResponse>(HttpRequestContext &context, std::string_view)>;

    namespace BasicAuthImpl
    {
        template <typename TValidator>
        concept CredentialValidator = std::invocable<TValidator &, std::string_view, std::string_view> &&
                                      std::convertible_to<std::invoke_result_t<TValidator &, std::string_view, std::string_view>, bool>;

        template <typename TOnSuccess>
        concept SuccessCallback = std::invocable<TOnSuccess &, std::string_view, std::string_view>;

        template <CredentialValidator TValidator>
        bool CheckBasicAuthCredentials(HttpRequestContext &context, TValidator &&validator)
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

            const std::string decodedCredentials = lumalink::http::util::WebUtility::Base64DecodeToString(authHeaderValue.substr(Prefix.size()));
            const std::string_view decodedView(decodedCredentials.data(), decodedCredentials.size());
            const std::size_t separatorIndex = decodedView.find(':');
            if (separatorIndex == std::string_view::npos)
            {
                return false;
            }

            const std::string_view username = decodedView.substr(0, separatorIndex);
            const std::string_view password = decodedView.substr(separatorIndex + 1);
            if (!std::invoke(validator, username, password))
            {
                return false;
            }

            context.items().emplace("BasicAuth::Username", std::string(username));
            context.items().emplace("BasicAuth::Password", std::string(password));
            return true;
        }

        template <CredentialValidator TValidator, SuccessCallback TOnSuccess>
        bool CheckBasicAuthCredentials(HttpRequestContext &context, TValidator &&validator, TOnSuccess &&onSuccess)
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

            const std::string decodedCredentials = lumalink::http::util::WebUtility::Base64DecodeToString(authHeaderValue.substr(Prefix.size()));
            const std::string_view decodedView(decodedCredentials.data(), decodedCredentials.size());
            const std::size_t separatorIndex = decodedView.find(':');
            if (separatorIndex == std::string_view::npos)
            {
                return false;
            }

            const std::string_view username = decodedView.substr(0, separatorIndex);
            const std::string_view password = decodedView.substr(separatorIndex + 1);
            if (!std::invoke(validator, username, password))
            {
                return false;
            }

            std::invoke(onSuccess, username, password);
            return true;
        }
    }

    std::unique_ptr<IHttpResponse> defaultOnFailure(HttpRequestContext &context, std::string_view realm);

    template <BasicAuthImpl::CredentialValidator TValidator>
    IHttpHandler::InterceptorCallback BasicAuth(TValidator &&validator, std::string_view realm = "Restricted Area",
                                                BasicAuthFailureCallback onFailure = defaultOnFailure)
    {
        using Validator = std::decay_t<TValidator>;

        std::string realmValue(realm);
        return [validator = Validator(std::forward<TValidator>(validator)), realmValue = std::move(realmValue), onFailure = std::move(onFailure)](HttpRequestContext &context, IHttpHandler::InvocationNext next) mutable -> IHttpHandler::HandlerResult
        {
            if (BasicAuthImpl::CheckBasicAuthCredentials(context, validator))
            {
                return next();
            }
            return IHttpHandler::HandlerResult::responseResult(onFailure(context, realmValue));
        };
    }

    template <BasicAuthImpl::CredentialValidator TValidator, BasicAuthImpl::SuccessCallback TOnSuccess>
    IHttpHandler::InterceptorCallback BasicAuth(TValidator &&validator, std::string_view realm,
                                                TOnSuccess &&onSuccess,
                                                BasicAuthFailureCallback onFailure = defaultOnFailure)
    {
        using Validator = std::decay_t<TValidator>;
        using OnSuccess = std::decay_t<TOnSuccess>;

        std::string realmValue(realm);
        return [validator = Validator(std::forward<TValidator>(validator)), realmValue = std::move(realmValue), onSuccess = OnSuccess(std::forward<TOnSuccess>(onSuccess)), onFailure = std::move(onFailure)](HttpRequestContext &context, IHttpHandler::InvocationNext next) mutable -> IHttpHandler::HandlerResult
        {
            if (BasicAuthImpl::CheckBasicAuthCredentials(context, validator, onSuccess))
            {
                return next();
            }
            return IHttpHandler::HandlerResult::responseResult(onFailure(context, realmValue));
        };
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(std::string_view expectedUsername, std::string_view expectedPassword, std::string_view realm = "Restricted Area",
                                                       BasicAuthFailureCallback onFailure = defaultOnFailure)
    {
        const std::string expectedUsernameValue(expectedUsername);
        const std::string expectedPasswordValue(expectedPassword);

        return BasicAuth(
            [expectedUsernameValue, expectedPasswordValue](std::string_view foundUsername, std::string_view foundPassword)
            {
                return foundUsername == expectedUsernameValue && foundPassword == expectedPasswordValue;
            },
            realm,
            std::move(onFailure));
    }

    template <BasicAuthImpl::SuccessCallback TOnSuccess>
    IHttpHandler::InterceptorCallback BasicAuth(std::string_view expectedUsername, std::string_view expectedPassword, std::string_view realm,
                                                TOnSuccess &&onSuccess,
                                                BasicAuthFailureCallback onFailure = defaultOnFailure)
    {
        const std::string expectedUsernameValue(expectedUsername);
        const std::string expectedPasswordValue(expectedPassword);

        return BasicAuth(
            [expectedUsernameValue, expectedPasswordValue](std::string_view foundUsername, std::string_view foundPassword)
            {
                return foundUsername == expectedUsernameValue && foundPassword == expectedPasswordValue;
            },
            realm,
            std::forward<TOnSuccess>(onSuccess),
            std::move(onFailure));
    }
} // namespace lumalink::http::routing





