#pragma once

#include <memory>

#include "ConnectionSession.h"
#include "PipelineError.h"

#include "../compat/ByteStream.h"

namespace httpadv::v1::pipeline
{
    using httpadv::v1::server::IConnectionSession;
    using httpadv::v1::transport::IByteSource;

    struct RequestHandlingResult
    {
        enum class Kind
        {
            None,
            Response,
            Upgrade,
            NoResponse,
            Error
        };

        Kind kind = Kind::None;
        std::unique_ptr<IByteSource> responseStream;
        std::unique_ptr<IConnectionSession> upgradedSession;
        PipelineError error;

        static RequestHandlingResult response(std::unique_ptr<IByteSource> stream)
        {
            RequestHandlingResult result;
            result.kind = Kind::Response;
            result.responseStream = std::move(stream);
            return result;
        }

        static RequestHandlingResult upgrade(std::unique_ptr<IConnectionSession> session)
        {
            RequestHandlingResult result;
            result.kind = Kind::Upgrade;
            result.upgradedSession = std::move(session);
            return result;
        }

        static RequestHandlingResult noResponse()
        {
            RequestHandlingResult result;
            result.kind = Kind::NoResponse;
            return result;
        }

        static RequestHandlingResult errorResult(PipelineError pipelineError)
        {
            RequestHandlingResult result;
            result.kind = Kind::Error;
            result.error = pipelineError;
            return result;
        }

        bool hasValue() const
        {
            return kind != Kind::None;
        }
    };
}