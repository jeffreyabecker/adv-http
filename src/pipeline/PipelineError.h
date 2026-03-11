#pragma once
#include <cstdint>

namespace HttpServerAdvanced
{
    enum class PipelineErrorCode : int32_t
    {
        None = 0,
        ParseError = 1,
        InvalidMethod = 2,
        InvalidVersion = 3,
        InvalidHeader = 4,
        HeaderTooLarge = 5,
        UriTooLong = 6,
        BodyTooLarge = 7,
        BoundaryMissing = 8,
        Timeout = 9,
        ClientDisconnected = 10,
        UnsupportedMediaType = 11,
        BadRequest = 12,
        InternalError = 100,
        StrictError = 2,
        CrExpectedError = 25,
        LfExpectedError = 3,
        UnexpectedContentLengthError = 4,
        UnexpectedSpaceError = 30,
        ClosedConnectionError = 5,
        InvalidMethodError = 6,
        InvalidUrlError = 7,
        InvalidConstantError = 8,
        InvalidVersionError = 9,
        InvalidHeaderTokenError = 10,
        InvalidContentLengthError = 11,
        InvalidChunkSizeError = 12,
        InvalidStatusError = 13,
        InvalidEofStateError = 14,
        InvalidTransferEncodingError = 15,
        Unknown = 101
    };

    const char *PipelineErrorName(PipelineErrorCode c);

    const char *PipelineErrorMessage(PipelineErrorCode c);

    class PipelineError
    {
    public:
        PipelineError() : code_(PipelineErrorCode::None) {}
        explicit PipelineError(PipelineErrorCode c) : code_(c) {}

        PipelineErrorCode code() const { return code_; }
        int32_t codeInt() const { return static_cast<int32_t>(code_); }
        const char *name() const { return PipelineErrorName(code_); }
        const char *message() const { return PipelineErrorMessage(code_); }

    private:
        PipelineErrorCode code_;
    };
} // namespace HttpServerAdvanced
