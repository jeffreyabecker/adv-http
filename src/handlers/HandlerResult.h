#pragma once

#include "../pipeline/ConnectionSession.h"
#include "../response/IHttpResponse.h"

#include <memory>

namespace HttpServerAdvanced
{
    struct HandlerResult
    {
        enum class Kind
        {
            None,
            Response,
            Upgrade
        };

        Kind kind = Kind::None;
        std::unique_ptr<IHttpResponse> response;
        std::unique_ptr<IConnectionSession> upgradedSession;

        HandlerResult() = default;

        HandlerResult(std::nullptr_t)
        {
        }

        HandlerResult(std::unique_ptr<IHttpResponse> httpResponse)
            : kind(httpResponse ? Kind::Response : Kind::None),
              response(std::move(httpResponse))
        {
        }

        static HandlerResult responseResult(std::unique_ptr<IHttpResponse> httpResponse)
        {
            return HandlerResult(std::move(httpResponse));
        }

        static HandlerResult upgradeResult(std::unique_ptr<IConnectionSession> session)
        {
            HandlerResult result;
            result.kind = session ? Kind::Upgrade : Kind::None;
            result.upgradedSession = std::move(session);
            return result;
        }

        bool hasValue() const
        {
            return kind != Kind::None;
        }

        bool isResponse() const
        {
            return kind == Kind::Response;
        }

        bool isUpgrade() const
        {
            return kind == Kind::Upgrade;
        }

        IHttpResponse *get() const
        {
            return response.get();
        }

        explicit operator bool() const
        {
            return hasValue();
        }
    };
}