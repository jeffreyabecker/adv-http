#pragma once

#include <Arduino.h>
#include "../core/Defines.h"
#include "../core/HttpRequest.h"
#include "../response/HttpResponse.h"
#include "../core/HttpStatus.h"
#include "../util/HttpUtility.h"
#include "../handlers/HttpHandler.h"
#include "../response/StringResponse.h"
#include <memory>
#include <string>
#include <string_view>

namespace HttpServerAdvanced
{
    namespace BasicAuthImpl
    {
        inline String toArduinoString(std::string_view value)
        {
            return HttpHeaderDetail::ToArduinoString(value);
        }

        inline std::string toStdString(const String &value)
        {
            return std::string(value.c_str(), value.length());
        }

        inline bool CheckBasicAuthCredentials(HttpRequest &context, std::function<bool(std::string_view, std::string_view)> validator,
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
            const std::string decodedCredentials = HttpServerAdvanced::WebUtility::Base64DecodeToStdString(authHeaderValue.substr(Prefix.size()));
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
                context.items().emplace("BasicAuth::Username", toArduinoString(username));
                context.items().emplace("BasicAuth::Password", toArduinoString(password));
            }
            return true;
        }
    }

    inline std::unique_ptr<IHttpResponse> defaultOnFailure(HttpRequest &context, std::string_view realm)
    {
        return StringResponse::create(
            HttpStatus::Unauthorized(),
            std::string_view("Unauthorized"),
            {HttpHeader(std::string_view(HttpHeaderNames::WwwAuthenticate), std::string("Basic realm=\"") + std::string(realm) + "\"")});
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(std::function<bool(std::string_view, std::string_view)> validator, std::string_view realm = "Restricted Area",
                                                       std::function<void(std::string_view, std::string_view)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, std::string_view)> onFailure = defaultOnFailure)
    {
        std::string realmValue(realm);

        return [validator = std::move(validator), realmValue = std::move(realmValue), onSuccess = std::move(onSuccess), onFailure = std::move(onFailure)](HttpRequest &context, IHttpHandler::InvocationCallback next)
        {
            if (BasicAuthImpl::CheckBasicAuthCredentials(context, validator, onSuccess))
            {
                return next(context);
            }
            return onFailure(context, realmValue);
        };
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(std::function<bool(const String &, const String &)> validator, const String &realm = "Restricted Area",
                                                       std::function<void(const String &, const String &)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, const String &)> onFailure = nullptr)
    {
        std::function<void(std::string_view, std::string_view)> wrappedOnSuccess;
        if (onSuccess)
        {
            wrappedOnSuccess = [onSuccess](std::string_view username, std::string_view password)
            {
                onSuccess(BasicAuthImpl::toArduinoString(username), BasicAuthImpl::toArduinoString(password));
            };
        }

        std::function<std::unique_ptr<IHttpResponse>(HttpRequest &, std::string_view)> wrappedOnFailure;
        if (onFailure)
        {
            wrappedOnFailure = [onFailure](HttpRequest &context, std::string_view realmValue)
            {
                return onFailure(context, BasicAuthImpl::toArduinoString(realmValue));
            };
        }
        else
        {
            wrappedOnFailure = [](HttpRequest &context, std::string_view realmValue)
            {
                return defaultOnFailure(context, realmValue);
            };
        }

        return BasicAuth(
            [validator](std::string_view foundUsername, std::string_view foundPassword)
            {
                return validator(BasicAuthImpl::toArduinoString(foundUsername), BasicAuthImpl::toArduinoString(foundPassword));
            },
            std::string_view(realm.c_str(), realm.length()),
            std::move(wrappedOnSuccess),
            std::move(wrappedOnFailure));
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(std::string_view expectedUsername, std::string_view expectedPassword, std::string_view realm = "Restricted Area",
                                                       std::function<void(std::string_view, std::string_view)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, std::string_view)> onFailure = defaultOnFailure)
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

    inline IHttpHandler::InterceptorCallback BasicAuth(const String &expectedUsername, const String &expectedPassword, const String &realm = "Restricted Area",
                                                       std::function<void(const String &, const String &)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, const String &)> onFailure = nullptr)
    {
        std::function<void(std::string_view, std::string_view)> wrappedOnSuccess;
        if (onSuccess)
        {
            wrappedOnSuccess = [onSuccess](std::string_view username, std::string_view password)
            {
                onSuccess(BasicAuthImpl::toArduinoString(username), BasicAuthImpl::toArduinoString(password));
            };
        }

        std::function<std::unique_ptr<IHttpResponse>(HttpRequest &, std::string_view)> wrappedOnFailure;
        if (onFailure)
        {
            wrappedOnFailure = [onFailure](HttpRequest &context, std::string_view realmValue)
            {
                return onFailure(context, BasicAuthImpl::toArduinoString(realmValue));
            };
        }
        else
        {
            wrappedOnFailure = [](HttpRequest &context, std::string_view realmValue)
            {
                return defaultOnFailure(context, realmValue);
            };
        }

        return BasicAuth(
            std::string_view(expectedUsername.c_str(), expectedUsername.length()),
            std::string_view(expectedPassword.c_str(), expectedPassword.length()),
            std::string_view(realm.c_str(), realm.length()),
            std::move(wrappedOnSuccess),
            std::move(wrappedOnFailure));
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(const char *expectedUsername, const char *expectedPassword, const char *realm = "Restricted Area",
                                                       std::function<void(const String &, const String &)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, const String &)> onFailure = nullptr)
    {
        return BasicAuth(String(expectedUsername != nullptr ? expectedUsername : ""),
                         String(expectedPassword != nullptr ? expectedPassword : ""),
                         String(realm != nullptr ? realm : "Restricted Area"),
                         onSuccess,
                         onFailure);
    }
} // namespace HttpServerAdvanced





