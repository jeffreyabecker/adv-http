#pragma once
#ifndef HTTPADV_V1_STREAMS_URISTREAM_H
#define HTTPADV_V1_STREAMS_URISTREAM_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../transport/ByteStream.h"
#include "../util/Span.h"

namespace httpadv::v1::streams
{
    using lumalink::platform::buffers::AvailableResult;
    using lumalink::platform::buffers::IByteSource;
    using lumalink::platform::buffers::StdStringByteSource;

    /**
     * @brief A stream that decodes URI-encoded data (percent-encoding and + for space).
     */
    class UriDecodingStream : public IByteSource
    {
    private:
        std::unique_ptr<IByteSource> innerStream_;
        enum class State
        {
            Normal,
            Percent1,
            Percent2
        } state_ = State::Normal;
        char percentBuffer_[2];
        size_t percentIndex_ = 0;

    public:
        UriDecodingStream(const char *uri);
        UriDecodingStream(const uint8_t *uri, size_t length);
        explicit UriDecodingStream(std::unique_ptr<IByteSource> innerStream);
        AvailableResult available() override;
        size_t read(lumalink::span<uint8_t> buffer) override;
        size_t peek(lumalink::span<uint8_t> buffer) override;

    private:
        int readSingleByte();
        int peekSingleByte();
    };

    /**
     * @brief A stream that encodes data using URI percent-encoding.
     */
    class UriEncodingStream : public IByteSource
    {
    private:
        std::unique_ptr<IByteSource> innerStream_;
        enum class State
        {
            Normal,
            EncodedPercent,
            EncodedHex1,
            EncodedHex2
        } state_ = State::Normal;
        char encodedBuffer_[3]; // "%XX"
        size_t encodedIndex_ = 0;

    public:
        UriEncodingStream(const char *uri);
        UriEncodingStream(const uint8_t *uri, size_t length);
        explicit UriEncodingStream(std::unique_ptr<IByteSource> innerStream);
        AvailableResult available() override;
        size_t read(lumalink::span<uint8_t> buffer) override;
        size_t peek(lumalink::span<uint8_t> buffer) override;

    private:
        int readSingleByte();
        int peekSingleByte();
    };

    class FormEncodingStream : public StdStringByteSource
    {
    public:
        explicit FormEncodingStream(std::vector<std::pair<std::string, std::string>> &&data);
    };
}

#endif // HTTPADV_V1_STREAMS_URISTREAM_H
