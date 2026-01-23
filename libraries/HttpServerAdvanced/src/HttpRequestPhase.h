#pragma once

namespace HttpServerAdvanced
{
    using HttpRequestPhaseFlags = uint8_t;
    namespace HttpRequestPhase
    {
        constexpr uint8_t CompletedStartingLine = 1 << 0;
        constexpr uint8_t CompletedReadingHeaders = 1 << 1;
        constexpr uint8_t BeginReadingBody = 1 << 2;
        constexpr uint8_t CompletedReadingMessage = 1 << 3;
        constexpr uint8_t WritingResponseStarted = 1 << 4;
        constexpr uint8_t CompletedWritingResponse = 1 << 5;

        constexpr uint8_t All = CompletedStartingLine | CompletedReadingHeaders | BeginReadingBody | CompletedReadingMessage | WritingResponseStarted | CompletedWritingResponse;
    }
}
