#include "PipelineError.h"

namespace httpadv::v1::pipeline
{
    const char *PipelineErrorName(PipelineErrorCode c)
    {
        switch (c)
        {
        case PipelineErrorCode::None:
            return "None";
        case PipelineErrorCode::ParseError:
            return "ParseError";
        case PipelineErrorCode::InternalError:
            return "InternalError";
        case PipelineErrorCode::StrictError:
            return "StrictError";
        case PipelineErrorCode::CrExpectedError:
            return "CrExpectedError";
        case PipelineErrorCode::LfExpectedError:
            return "LfExpectedError";
        case PipelineErrorCode::UnexpectedContentLengthError:
            return "UnexpectedContentLengthError";
        case PipelineErrorCode::UnexpectedSpaceError:
            return "UnexpectedSpaceError";
        case PipelineErrorCode::ClosedConnectionError:
            return "ClosedConnectionError";
        case PipelineErrorCode::InvalidMethodError:
            return "InvalidMethodError";
        case PipelineErrorCode::InvalidUrlError:
            return "InvalidUrlError";
        case PipelineErrorCode::InvalidConstantError:
            return "InvalidConstantError";
        case PipelineErrorCode::InvalidVersionError:
            return "InvalidVersionError";
        case PipelineErrorCode::InvalidHeaderTokenError:
            return "InvalidHeaderTokenError";
        case PipelineErrorCode::InvalidContentLengthError:
            return "InvalidContentLengthError";
        case PipelineErrorCode::InvalidChunkSizeError:
            return "InvalidChunkSizeError";
        case PipelineErrorCode::InvalidStatusError:
            return "InvalidStatusError";
        case PipelineErrorCode::InvalidEofStateError:
            return "InvalidEofStateError";
        case PipelineErrorCode::InvalidTransferEncodingError:
            return "InvalidTransferEncodingError";
        case PipelineErrorCode::HeaderTooLarge:
            return "HeaderTooLarge";
        case PipelineErrorCode::UriTooLong:
            return "UriTooLong";
        case PipelineErrorCode::BodyTooLarge:
            return "BodyTooLarge";
        case PipelineErrorCode::BoundaryMissing:
            return "BoundaryMissing";
        case PipelineErrorCode::Timeout:
            return "Timeout";
        case PipelineErrorCode::ClientDisconnected:
            return "ClientDisconnected";
        case PipelineErrorCode::UnsupportedMediaType:
            return "UnsupportedMediaType";
        case PipelineErrorCode::BadRequest:
            return "BadRequest";
        case PipelineErrorCode::Unknown:
        default:
            return "Unknown";
        }
    }

    const char *PipelineErrorMessage(PipelineErrorCode c)
    {
        switch (c)
        {
        case PipelineErrorCode::None:
            return "No error";
        case PipelineErrorCode::ParseError:
            return "Parsing of the HTTP message failed";
        case PipelineErrorCode::InternalError:
            return "Internal server error";
        case PipelineErrorCode::StrictError:
            return "Strict mode error";
        case PipelineErrorCode::CrExpectedError:
            return "Expected carriage return (CR) character";
        case PipelineErrorCode::LfExpectedError:
            return "Expected line feed (LF) character";
        case PipelineErrorCode::UnexpectedContentLengthError:
            return "Unexpected Content-Length header";
        case PipelineErrorCode::UnexpectedSpaceError:
            return "Unexpected space character";
        case PipelineErrorCode::ClosedConnectionError:
            return "Connection closed unexpectedly";
        case PipelineErrorCode::InvalidMethodError:
            return "Invalid HTTP method";
        case PipelineErrorCode::InvalidUrlError:
            return "Invalid URL format";
        case PipelineErrorCode::InvalidConstantError:
            return "Invalid constant in HTTP message";
        case PipelineErrorCode::InvalidVersionError:
            return "Invalid HTTP version";
        case PipelineErrorCode::InvalidHeaderTokenError:
            return "Invalid header token";
        case PipelineErrorCode::InvalidContentLengthError:
            return "Invalid Content-Length value";
        case PipelineErrorCode::InvalidChunkSizeError:
            return "Invalid chunk size in chunked encoding";
        case PipelineErrorCode::InvalidStatusError:
            return "Invalid status code";
        case PipelineErrorCode::InvalidEofStateError:
            return "Invalid end-of-file state";
        case PipelineErrorCode::InvalidTransferEncodingError:
            return "Invalid Transfer-Encoding header";
        case PipelineErrorCode::HeaderTooLarge:
            return "Header size limit exceeded";
        case PipelineErrorCode::UriTooLong:
            return "Request URI too long";
        case PipelineErrorCode::BodyTooLarge:
            return "Request body exceeds allowed size";
        case PipelineErrorCode::BoundaryMissing:
            return "Multipart boundary missing or invalid";
        case PipelineErrorCode::Timeout:
            return "Timeout during request processing";
        case PipelineErrorCode::ClientDisconnected:
            return "Client disconnected";
        case PipelineErrorCode::UnsupportedMediaType:
            return "Unsupported media type";
        case PipelineErrorCode::BadRequest:
            return "Bad Request";
        case PipelineErrorCode::Unknown:
        default:
            return "Unknown pipeline error";
        }
    }
} // namespace HttpServerAdvanced
