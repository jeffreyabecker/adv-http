#pragma once

#include <expected>
#include <memory>

#include "ConnectionSession.h"
#include "PipelineError.h"

#include "LumaLinkPlatform.h"

namespace lumalink::http::pipeline
{
    using lumalink::http::server::IConnectionSession;
    using lumalink::platform::buffers::IByteSource;

    struct RequestHandlingSuccess
    {
        enum class Kind
        {
            None,
            Response,
            Upgrade,
            NoResponse
        };

        Kind kind = Kind::None;
        std::unique_ptr<IByteSource> responseStream;
        std::unique_ptr<IConnectionSession> upgradedSession;
    };

    using RequestHandlingResult = std::expected<RequestHandlingSuccess, PipelineError>;

    inline RequestHandlingResult EmptyRequestHandlingResult()
    {
        return RequestHandlingSuccess{};
    }

    inline RequestHandlingResult ResponseRequestHandlingResult(std::unique_ptr<IByteSource> stream)
    {
        RequestHandlingSuccess result;
        result.kind = RequestHandlingSuccess::Kind::Response;
        result.responseStream = std::move(stream);
        return result;
    }

    inline RequestHandlingResult UpgradeRequestHandlingResult(std::unique_ptr<IConnectionSession> session)
    {
        RequestHandlingSuccess result;
        result.kind = RequestHandlingSuccess::Kind::Upgrade;
        result.upgradedSession = std::move(session);
        return result;
    }

    inline RequestHandlingResult NoResponseRequestHandlingResult()
    {
        RequestHandlingSuccess result;
        result.kind = RequestHandlingSuccess::Kind::NoResponse;
        return result;
    }

    inline RequestHandlingResult ErrorRequestHandlingResult(PipelineError pipelineError)
    {
        return std::unexpected(pipelineError);
    }

    inline bool HasPendingRequestHandlingValue(const RequestHandlingResult& result)
    {
        return result.has_value() && result->kind != RequestHandlingSuccess::Kind::None;
    }
}