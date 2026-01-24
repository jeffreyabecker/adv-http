#include "PipelineError.h"

namespace HttpServerAdvanced
{
    const char *PipelineErrorName(PipelineErrorCode c)
    {
        switch (c)
        {
        case PipelineErrorCode::None:
            return "None";
        case PipelineErrorCode::ParseError:
            return "ParseError";
        case PipelineErrorCode::InvalidMethod:
            return "InvalidMethod";
        case PipelineErrorCode::InvalidVersion:
            return "InvalidVersion";
        case PipelineErrorCode::InvalidHeader:
            return "InvalidHeader";
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
        case PipelineErrorCode::InternalError:
            return "InternalError";
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
        case PipelineErrorCode::InvalidMethod:
            return "Invalid HTTP method";
        case PipelineErrorCode::InvalidVersion:
            return "Unsupported HTTP version";
        case PipelineErrorCode::InvalidHeader:
            return "Malformed header or header parsing error";
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
        case PipelineErrorCode::InternalError:
            return "Internal server error";
        case PipelineErrorCode::Unknown:
        default:
            return "Unknown pipeline error";
        }
    }
} // namespace HttpServerAdvanced
