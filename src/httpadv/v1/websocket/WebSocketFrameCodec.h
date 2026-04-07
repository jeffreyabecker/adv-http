#pragma once

#include "LumaLinkPlatform.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace httpadv::v1::websocket
{
    using lumalink::span;

    enum class WebSocketOpcode : std::uint8_t
    {
        Continuation = 0x0,
        Text = 0x1,
        Binary = 0x2,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA
    };

    enum class WebSocketCloseCode : std::uint16_t
    {
        NormalClosure = 1000,
        GoingAway = 1001,
        ProtocolError = 1002,
        UnsupportedData = 1003,
        MessageTooBig = 1009,
        InternalError = 1011
    };

    enum class WebSocketCodecStatus
    {
        Ok,
        NeedMoreData,
        ProtocolError,
        BufferTooSmall
    };

    enum class WebSocketCodecError
    {
        None,
        InvalidReservedBits,
        InvalidOpcode,
        UnmaskedClientFrame,
        ControlFrameFragmented,
        ControlFrameTooLarge,
        MalformedExtendedPayloadLength,
        MalformedClosePayload,
        UnexpectedContinuationFrame,
        InterruptedContinuationSequence
    };

    struct WebSocketFrameHeader
    {
        bool fin = false;
        bool rsv1 = false;
        bool rsv2 = false;
        bool rsv3 = false;
        bool masked = false;
        WebSocketOpcode opcode = WebSocketOpcode::Continuation;
        std::uint64_t payloadLength = 0;
        std::array<std::uint8_t, 4> maskingKey = {0, 0, 0, 0};
    };

    struct WebSocketFrame
    {
        WebSocketFrameHeader header;
        std::vector<std::uint8_t> payload;
    };

    struct WebSocketParseResult
    {
        WebSocketCodecStatus status = WebSocketCodecStatus::NeedMoreData;
        WebSocketCodecError error = WebSocketCodecError::None;
        std::size_t bytesConsumed = 0;
        bool frameReady = false;
        WebSocketFrame frame;
    };

    struct WebSocketSerializeResult
    {
        WebSocketCodecStatus status = WebSocketCodecStatus::Ok;
        WebSocketCodecError error = WebSocketCodecError::None;
        std::size_t bytesWritten = 0;
    };

    class WebSocketFrameParser
    {
    public:
        explicit WebSocketFrameParser(bool requireMaskedClientFrames = true);

        WebSocketParseResult parse(span<const std::uint8_t> input);
        void reset();

    private:
        enum class Stage
        {
            BasicHeader,
            ExtendedLength,
            MaskingKey,
            Payload
        };

        static bool isControlOpcode(WebSocketOpcode opcode);
        static bool isDataOpcode(WebSocketOpcode opcode);
        static bool isKnownOpcode(std::uint8_t opcode);
        static std::uint64_t readBigEndian(span<const std::uint8_t> bytes);

        bool requireMaskedClientFrames_ = true;
        bool expectingContinuation_ = false;

        Stage stage_ = Stage::BasicHeader;
        WebSocketFrame currentFrame_;
        std::size_t payloadBytesRead_ = 0;

        std::uint8_t pendingLengthByteCount_ = 0;
        std::array<std::uint8_t, 8> pendingLengthBytes_ = {0, 0, 0, 0, 0, 0, 0, 0};
        std::uint8_t pendingMaskBytesRead_ = 0;
        std::uint8_t pendingBasicHeaderBytesRead_ = 0;
        std::array<std::uint8_t, 2> pendingBasicHeaderBytes_ = {0, 0};

        void resetCurrentFrame();
    };

    class WebSocketFrameSerializer
    {
    public:
        static std::size_t maxSerializedSize(std::size_t payloadLength);
        static WebSocketSerializeResult serialize(
            span<std::uint8_t> output,
            span<const std::uint8_t> payload,
            WebSocketOpcode opcode,
            bool fin = true);
    };
}
