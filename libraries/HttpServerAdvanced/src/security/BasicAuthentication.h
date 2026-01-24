#pragma once

#include <Arduino.h>
#include "../core/Defines.h"
#include "../server/HttpRequest.h"
#include "../response/HttpResponse.h"
#include "../core/HttpStatus.h"
#include "../util/HttpUtility.h"
#include "../handlers/HttpHandler.h"
#include "../response/StringResponse.h"
#include <memory>

namespace HttpServerAdvanced
{
    namespace BasicAuthImpl
    {

        bool CheckBasicAuthCredentials(HttpRequest &context, std::function<bool(const String &, const String &)> validator,
                                       const String &realm, std::function<void(const String &, const String &)> onSuccess)
        {
            auto authHeaderOpt = context.headers().find("Authorization");
            if (!authHeaderOpt.has_value())
            {
                return false;
            }
            auto authHeader = authHeaderOpt.value();
            const String prefix = "Basic ";
            if (!HttpServerAdvanced::StringUtil::startsWith(authHeader.value(), prefix, false))
            {
                return false;
            }
            String encodedCredentials = authHeader.value().substring(prefix.length());
            String decodedCredentials = HttpServerAdvanced::WebUtility::Base64DecodeToString(encodedCredentials);
            auto separatorIndex = decodedCredentials.indexOf(':');
            if (separatorIndex == -1)
            {
                return false;
            }
            String username = decodedCredentials.substring(0, separatorIndex);
            String password = decodedCredentials.substring(separatorIndex + 1);
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
                context.items().emplace("BasicAuth::Username", username);
                context.items().emplace("BasicAuth::Password", password);
            }
            return true;
        }
    }
    std::unique_ptr<IHttpResponse> defaultOnFailure(HttpRequest &context, const String &realm)
    {
        return StringResponse::create(
            HttpStatus::Unauthorized(),
            "Unauthorized",
            {HttpHeader::WwwAuthenticate(String("Basic realm=\"") + realm + "\"")});
    }

    IHttpHandler::InterceptorCallback BasicAuth(std::function<bool(const String &, const String &)> validator, const String &realm = "Restricted Area",
                                                std::function<void(const String &, const String &)> onSuccess = nullptr,
                                                std::function<std::unique_ptr<IHttpResponse>(HttpRequest &context, const String &)> onFailure = defaultOnFailure)
    {
        return [validator, &realm, onSuccess, onFailure](HttpRequest &context, IHttpHandler::InvocationCallback next)
        {
            if (BasicAuthImpl::CheckBasicAuthCredentials(context, validator, realm, onSuccess))
            {
                return next(context);
            }
            else
            {
                std::unique_ptr<IHttpResponse> response = onFailure(context, realm);
                return std::move(response);
            }
        };
    }

    IHttpHandler::InterceptorCallback BasicAuth(const String &expectedUsername, const String &expectedPassword, const String &realm = "Restricted Area",
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
} // namespace HttpServerAdvanced




