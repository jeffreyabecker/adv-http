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
#include <string_view>

namespace HttpServerAdvanced
{
    namespace BasicAuthImpl
    {
        inline String toArduinoString(std::string_view value)
        {
            return HttpHeaderDetail::ToArduinoString(value);
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
            String decodedCredentials = HttpServerAdvanced::WebUtility::Base64DecodeToString(authHeaderValue.data() + Prefix.size(), authHeaderValue.size() - Prefix.size());
            const std::string_view decodedView(decodedCredentials.c_str(), decodedCredentials.length());
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
    inline std::unique_ptr<IHttpResponse> defaultOnFailure(HttpRequest &context, const String &realm)
    {
        return StringResponse::create(
            HttpStatus::Unauthorized(),
            "Unauthorized",
            {HttpHeader::WwwAuthenticate(String("Basic realm=\"") + realm + "\"")});
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(std::function<bool(const String &, const String &)> validator, const String &realm = "Restricted Area",
                                                       std::function<void(const String &, const String &)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, const String &)> onFailure = defaultOnFailure)
    {
        String realmValue = realm;
        std::function<bool(std::string_view, std::string_view)> wrappedValidator = [validator](std::string_view foundUsername, std::string_view foundPassword)
        {
            return validator(BasicAuthImpl::toArduinoString(foundUsername), BasicAuthImpl::toArduinoString(foundPassword));
        };
        std::function<void(std::string_view, std::string_view)> wrappedOnSuccess;
        if (onSuccess)
        {
            wrappedOnSuccess = [onSuccess](std::string_view username, std::string_view password)
            {
                onSuccess(BasicAuthImpl::toArduinoString(username), BasicAuthImpl::toArduinoString(password));
            };
        }

        return [wrappedValidator, realmValue, wrappedOnSuccess, onFailure](HttpRequest &context, IHttpHandler::InvocationCallback next)
        {
            if (BasicAuthImpl::CheckBasicAuthCredentials(context, wrappedValidator, wrappedOnSuccess))
            {
                return next(context);
            }
            else
            {
                std::unique_ptr<IHttpResponse> response = onFailure(context, realmValue);
                return std::move(response);
            }
        };
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(const String &expectedUsername, const String &expectedPassword, const String &realm = "Restricted Area",
                                                       std::function<void(const String &, const String &)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, const String &)> onFailure = defaultOnFailure)
    {
        // Explicitly construct std::function to avoid overload ambiguity
        std::function<bool(const String &, const String &)> validator = [expectedUsername, expectedPassword](const String &foundUsername, const String &foundPassword)
        {
            return foundUsername == expectedUsername && foundPassword == expectedPassword;
        };
        return BasicAuth(validator, realm, onSuccess, onFailure);
    }

    inline IHttpHandler::InterceptorCallback BasicAuth(const char *expectedUsername, const char *expectedPassword, const char *realm = "Restricted Area",
                                                       std::function<void(const String &, const String &)> onSuccess = nullptr,
                                                       std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, const String &)> onFailure = defaultOnFailure)
    {
        return BasicAuth(String(expectedUsername != nullptr ? expectedUsername : ""),
                         String(expectedPassword != nullptr ? expectedPassword : ""),
                         String(realm != nullptr ? realm : "Restricted Area"),
                         onSuccess,
                         onFailure);
    }
} // namespace HttpServerAdvanced





