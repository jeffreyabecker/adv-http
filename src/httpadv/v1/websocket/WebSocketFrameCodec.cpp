#include "WebSocketFrameCodec.h"

#include <algorithm>

namespace httpadv::v1::websocket
{
    namespace
    {
        WebSocketParseResult makeProtocolError(WebSocketCodecError error, std::size_t bytesConsumed)
        {
            WebSocketParseResult result;
            result.status = WebSocketCodecStatus::ProtocolError;
            result.error = error;
            result.bytesConsumed = bytesConsumed;
            result.frameReady = false;
            return result;
        }
    }

    WebSocketFrameParser::WebSocketFrameParser(bool requireMaskedClientFrames)
        : requireMaskedClientFrames_(requireMaskedClientFrames)
    {
    }

    bool WebSocketFrameParser::isControlOpcode(WebSocketOpcode opcode)
    {
        return opcode == WebSocketOpcode::Close || opcode == WebSocketOpcode::Ping || opcode == WebSocketOpcode::Pong;
    }

    bool WebSocketFrameParser::isDataOpcode(WebSocketOpcode opcode)
    {
        return opcode == WebSocketOpcode::Text || opcode == WebSocketOpcode::Binary;
    }

    bool WebSocketFrameParser::isKnownOpcode(std::uint8_t opcode)
    {
        return opcode == static_cast<std::uint8_t>(WebSocketOpcode::Continuation) ||
               opcode == static_cast<std::uint8_t>(WebSocketOpcode::Text) ||
               opcode == static_cast<std::uint8_t>(WebSocketOpcode::Binary) ||
               opcode == static_cast<std::uint8_t>(WebSocketOpcode::Close) ||
               opcode == static_cast<std::uint8_t>(WebSocketOpcode::Ping) ||
               opcode == static_cast<std::uint8_t>(WebSocketOpcode::Pong);
    }

    std::uint64_t WebSocketFrameParser::readBigEndian(span<const std::uint8_t> bytes)
    {
        std::uint64_t value = 0;
        for (std::size_t i = 0; i < bytes.size(); ++i)
        {
            value = (value << 8U) | static_cast<std::uint64_t>(bytes[i]);
        }
        return value;
    }

    void WebSocketFrameParser::resetCurrentFrame()
    {
        currentFrame_ = WebSocketFrame();
        stage_ = Stage::BasicHeader;
        payloadBytesRead_ = 0;
        pendingLengthByteCount_ = 0;
        pendingMaskBytesRead_ = 0;
        pendingBasicHeaderBytesRead_ = 0;
        pendingLengthBytes_.fill(0);
        pendingBasicHeaderBytes_.fill(0);
    }

    void WebSocketFrameParser::reset()
    {
        expectingContinuation_ = false;
        resetCurrentFrame();
    }

    WebSocketParseResult WebSocketFrameParser::parse(span<const std::uint8_t> input)
    {
        WebSocketParseResult result;
        std::size_t offset = 0;

        while (offset < input.size())
        {
            if (stage_ == Stage::BasicHeader)
            {
                while (pendingBasicHeaderBytesRead_ < 2U && offset < input.size())
                {
                    pendingBasicHeaderBytes_[pendingBasicHeaderBytesRead_++] = input[offset++];
                }

                if (pendingBasicHeaderBytesRead_ < 2U)
                {
                    result.status = WebSocketCodecStatus::NeedMoreData;
                    result.bytesConsumed = offset;
                    return result;
                }

                const std::uint8_t first = pendingBasicHeaderBytes_[0];
                const std::uint8_t second = pendingBasicHeaderBytes_[1];
                pendingBasicHeaderBytesRead_ = 0;
                pendingBasicHeaderBytes_.fill(0);

                currentFrame_.header.fin = (first & 0x80U) != 0;
                currentFrame_.header.rsv1 = (first & 0x40U) != 0;
                currentFrame_.header.rsv2 = (first & 0x20U) != 0;
                currentFrame_.header.rsv3 = (first & 0x10U) != 0;
                if (currentFrame_.header.rsv1 || currentFrame_.header.rsv2 || currentFrame_.header.rsv3)
                {
                    return makeProtocolError(WebSocketCodecError::InvalidReservedBits, offset);
                }

                const std::uint8_t rawOpcode = static_cast<std::uint8_t>(first & 0x0FU);
                if (!isKnownOpcode(rawOpcode))
                {
                    return makeProtocolError(WebSocketCodecError::InvalidOpcode, offset);
                }

                currentFrame_.header.opcode = static_cast<WebSocketOpcode>(rawOpcode);
                currentFrame_.header.masked = (second & 0x80U) != 0;
                if (requireMaskedClientFrames_ && !currentFrame_.header.masked)
                {
                    return makeProtocolError(WebSocketCodecError::UnmaskedClientFrame, offset);
                }

                const std::uint8_t payloadLen7 = static_cast<std::uint8_t>(second & 0x7FU);
                if (payloadLen7 <= 125U)
                {
                    currentFrame_.header.payloadLength = payloadLen7;
                    stage_ = currentFrame_.header.masked ? Stage::MaskingKey : Stage::Payload;
                }
                else
                {
                    pendingLengthByteCount_ = payloadLen7 == 126U ? 2U : 8U;
                    stage_ = Stage::ExtendedLength;
                }
            }

            if (stage_ == Stage::ExtendedLength)
            {
                while (pendingLengthByteCount_ > 0U && offset < input.size())
                {
                    const std::size_t index = static_cast<std::size_t>((pendingLengthBytes_.size() - pendingLengthByteCount_));
                    pendingLengthBytes_[index] = input[offset++];
                    --pendingLengthByteCount_;
                }

                if (pendingLengthByteCount_ > 0U)
                {
                    result.status = WebSocketCodecStatus::NeedMoreData;
                    result.bytesConsumed = offset;
                    return result;
                }

                std::uint64_t payloadLength = 0;
                if ((pendingLengthBytes_[0] == 0) && (pendingLengthBytes_[1] == 0) && pendingLengthBytes_[2] == 0 && pendingLengthBytes_[3] == 0 && pendingLengthBytes_[4] == 0 && pendingLengthBytes_[5] == 0)
                {
                    payloadLength = readBigEndian(span<const std::uint8_t>(pendingLengthBytes_.data() + 6, 2));
                }
                else
                {
                    payloadLength = readBigEndian(span<const std::uint8_t>(pendingLengthBytes_.data(), 8));
                }

                const bool parsedFromTwoBytes = pendingLengthBytes_[2] == 0 && pendingLengthBytes_[3] == 0 && pendingLengthBytes_[4] == 0 && pendingLengthBytes_[5] == 0;
                if (parsedFromTwoBytes && payloadLength < 126U)
                {
                    return makeProtocolError(WebSocketCodecError::MalformedExtendedPayloadLength, offset);
                }
                if (!parsedFromTwoBytes)
                {
                    if ((pendingLengthBytes_[0] & 0x80U) != 0U || payloadLength < 65536U)
                    {
                        return makeProtocolError(WebSocketCodecError::MalformedExtendedPayloadLength, offset);
                    }
                }

                currentFrame_.header.payloadLength = payloadLength;
                stage_ = currentFrame_.header.masked ? Stage::MaskingKey : Stage::Payload;
                pendingLengthBytes_.fill(0);
            }

            if (stage_ == Stage::MaskingKey)
            {
                while (pendingMaskBytesRead_ < 4U && offset < input.size())
                {
                    currentFrame_.header.maskingKey[pendingMaskBytesRead_++] = input[offset++];
                }

                if (pendingMaskBytesRead_ < 4U)
                {
                    result.status = WebSocketCodecStatus::NeedMoreData;
                    result.bytesConsumed = offset;
                    return result;
                }

                stage_ = Stage::Payload;
            }

            if (stage_ == Stage::Payload)
            {
                const std::size_t remainingPayload = static_cast<std::size_t>(currentFrame_.header.payloadLength - payloadBytesRead_);
                const std::size_t available = input.size() - offset;
                const std::size_t toRead = (std::min)(remainingPayload, available);
                if (toRead > 0)
                {
                    const std::size_t oldSize = currentFrame_.payload.size();
                    currentFrame_.payload.resize(oldSize + toRead);
                    for (std::size_t i = 0; i < toRead; ++i)
                    {
                        std::uint8_t value = input[offset + i];
                        if (currentFrame_.header.masked)
                        {
                            value ^= currentFrame_.header.maskingKey[(payloadBytesRead_ + i) % 4U];
                        }
                        currentFrame_.payload[oldSize + i] = value;
                    }
                    offset += toRead;
                    payloadBytesRead_ += toRead;
                }

                if (payloadBytesRead_ < currentFrame_.header.payloadLength)
                {
                    result.status = WebSocketCodecStatus::NeedMoreData;
                    result.bytesConsumed = offset;
                    return result;
                }

                const WebSocketOpcode opcode = currentFrame_.header.opcode;
                const bool isControl = isControlOpcode(opcode);
                if (isControl)
                {
                    if (!currentFrame_.header.fin)
                    {
                        return makeProtocolError(WebSocketCodecError::ControlFrameFragmented, offset);
                    }
                    if (currentFrame_.header.payloadLength > 125U)
                    {
                        return makeProtocolError(WebSocketCodecError::ControlFrameTooLarge, offset);
                    }
                    if (opcode == WebSocketOpcode::Close && currentFrame_.header.payloadLength == 1U)
                    {
                        return makeProtocolError(WebSocketCodecError::MalformedClosePayload, offset);
                    }
                }

                if (opcode == WebSocketOpcode::Continuation)
                {
                    if (!expectingContinuation_)
                    {
                        return makeProtocolError(WebSocketCodecError::UnexpectedContinuationFrame, offset);
                    }
                    if (currentFrame_.header.fin)
                    {
                        expectingContinuation_ = false;
                    }
                }
                else if (isDataOpcode(opcode))
                {
                    if (expectingContinuation_)
                    {
                        return makeProtocolError(WebSocketCodecError::InterruptedContinuationSequence, offset);
                    }
                    if (!currentFrame_.header.fin)
                    {
                        expectingContinuation_ = true;
                    }
                }

                result.status = WebSocketCodecStatus::Ok;
                result.error = WebSocketCodecError::None;
                result.bytesConsumed = offset;
                result.frameReady = true;
                result.frame = std::move(currentFrame_);

                resetCurrentFrame();
                return result;
            }
        }

        result.status = WebSocketCodecStatus::NeedMoreData;
        result.bytesConsumed = offset;
        return result;
    }

    std::size_t WebSocketFrameSerializer::maxSerializedSize(std::size_t payloadLength)
    {
        if (payloadLength <= 125U)
        {
            return 2U + payloadLength;
        }
        if (payloadLength <= 65535U)
        {
            return 4U + payloadLength;
        }
        return 10U + payloadLength;
    }

    WebSocketSerializeResult WebSocketFrameSerializer::serialize(
        span<std::uint8_t> output,
        span<const std::uint8_t> payload,
        WebSocketOpcode opcode,
        bool fin)
    {
        WebSocketSerializeResult result;
        const bool isControl = opcode == WebSocketOpcode::Close || opcode == WebSocketOpcode::Ping || opcode == WebSocketOpcode::Pong;
        if (isControl && (!fin || payload.size() > 125U))
        {
            result.status = WebSocketCodecStatus::ProtocolError;
            result.error = !fin ? WebSocketCodecError::ControlFrameFragmented : WebSocketCodecError::ControlFrameTooLarge;
            return result;
        }

        const std::size_t requiredSize = maxSerializedSize(payload.size());
        if (output.size() < requiredSize)
        {
            result.status = WebSocketCodecStatus::BufferTooSmall;
            return result;
        }

        std::size_t offset = 0;
        output[offset++] = static_cast<std::uint8_t>((fin ? 0x80U : 0x00U) | static_cast<std::uint8_t>(opcode));

        if (payload.size() <= 125U)
        {
            output[offset++] = static_cast<std::uint8_t>(payload.size());
        }
        else if (payload.size() <= 65535U)
        {
            output[offset++] = 126U;
            output[offset++] = static_cast<std::uint8_t>((payload.size() >> 8U) & 0xFFU);
            output[offset++] = static_cast<std::uint8_t>(payload.size() & 0xFFU);
        }
        else
        {
            output[offset++] = 127U;
            const std::uint64_t length = static_cast<std::uint64_t>(payload.size());
            for (int shift = 56; shift >= 0; shift -= 8)
            {
                output[offset++] = static_cast<std::uint8_t>((length >> static_cast<std::uint64_t>(shift)) & 0xFFU);
            }
        }

        for (std::size_t i = 0; i < payload.size(); ++i)
        {
            output[offset + i] = payload[i];
        }

        result.status = WebSocketCodecStatus::Ok;
        result.bytesWritten = requiredSize;
        return result;
    }
}
