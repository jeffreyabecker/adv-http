#pragma once
#include <Arduino.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Streams.h"

namespace HttpServerAdvanced
{
    /**
     * @brief A stream that decodes URI-encoded data (percent-encoding and + for space).
     */
    class UriDecodingStream : public ReadStream
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
        UriDecodingStream(const String &uri);
        UriDecodingStream(const char *uri);
        UriDecodingStream(const uint8_t *uri, size_t length);
        explicit UriDecodingStream(std::unique_ptr<IByteSource> innerStream);
        AvailableResult available() override;

    protected:
        int readSingleByte() override;
        int peekSingleByte() override;
    };

    /**
     * @brief A stream that encodes data using URI percent-encoding.
     */
    class UriEncodingStream : public ReadStream
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
        UriEncodingStream(const String &uri);
        UriEncodingStream(const char *uri);
        UriEncodingStream(const uint8_t *uri, size_t length);
        explicit UriEncodingStream(std::unique_ptr<IByteSource> innerStream);
        AvailableResult available() override;

    protected:
        int readSingleByte() override;
        int peekSingleByte() override;
    };

    class FormEncodingStream : public StdStringByteSource
    {
    public:
        explicit FormEncodingStream(std::vector<std::pair<std::string, std::string>> &&data);
        explicit FormEncodingStream(std::vector<std::pair<String, String>> &&data);
    };
}
