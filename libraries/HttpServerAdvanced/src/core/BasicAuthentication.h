#pragma once

#include <Arduino.h>
#include "./HttpContext.h"
#include "./HttpResponse.h"
#include "./HttpStatus.h"
#include "../HttpUtility.h"
#include "./HttpHandler.h"
#include <memory>

namespace HttpServerAdvanced::Core
{
    namespace BasicAuthImpl
    {

        bool CheckBasicAuthCredentials(HttpContext &context, const String &expectedUsername, const String &expectedPassword, const String &realm, std::function<void(const String &, const String &)> onSuccess)
        {
            auto authHeaderOpt = context.request().headers().find("Authorization");
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
            String decodedCredentials = HttpServerAdvanced::HttpUtility::base64DecodeToString(encodedCredentials);
            auto separatorIndex = decodedCredentials.indexOf(':');
            if (separatorIndex == -1)
            {
                return false;
            }
            String username = decodedCredentials.substring(0, separatorIndex);
            String password = decodedCredentials.substring(separatorIndex + 1);
            if (username != expectedUsername || password != expectedPassword)
            {
                return false;
            }
            onSuccess(username, password);
            return true;
        }
    }

    IHttpHandler::InterceptorCallback BasicAuth(const String &expectedUsername, const String &expectedPassword, const String &realm = "Restricted Area", std::function<void(const String &, const String &)> onSuccess = nullptr)
    {
        return [&expectedUsername, &expectedPassword, &realm, &onSuccess](HttpContext &context, IHttpHandler::InvocationCallback next)
        {
            bool authorized = BasicAuthImpl::CheckBasicAuthCredentials(context, expectedUsername, expectedPassword, realm, onSuccess);
            if (authorized)
            {
                return next(context);
            }
            else
            {
                return HttpResponse::create(HttpStatus::Unauthorized(),
                                            "text/plain", "Unauthorized", HttpHeaders::WwwAuthenticate("Basic realm=\"" + realm + "\""));
            }
        };
    }
} // namespace HttpServerAdvanced::Core