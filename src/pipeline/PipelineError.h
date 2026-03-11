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
        StrictError = 101,
        CrExpectedError = 102,
        LfExpectedError = 103,
        UnexpectedContentLengthError = 104,
        UnexpectedSpaceError = 105,
        ClosedConnectionError = 106,
        InvalidMethodError = 107,
        InvalidUrlError = 108,
        InvalidConstantError = 109,
        InvalidVersionError = 110,
        InvalidHeaderTokenError = 111,
        InvalidContentLengthError = 112,
        InvalidChunkSizeError = 113,
        InvalidStatusError = 114,
        InvalidEofStateError = 115,
        InvalidTransferEncodingError = 116,
        Unknown = 199
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
