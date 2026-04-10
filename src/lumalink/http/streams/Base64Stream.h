#pragma once
#include <exception>
#include <span>
#include "LumaLinkPlatform.h"
namespace lumalink::http::streams
{
    using lumalink::platform::buffers::ByteAvailability;
    using lumalink::platform::buffers::IByteSource;

    constexpr const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    constexpr const char base64_url_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    class Base64DecoderStream : public IByteSource
    {
    private:
        std::unique_ptr<IByteSource> underlyingStream_;

        const char *dictionary_;
        uint8_t buffer_[3];
        int bufferPos_ = 0;
        int bufferSize_ = 0;
        bool eof_ = false;

        size_t totalLength_ = 0;   // Total encoded length if known
        size_t decodedLength_ = 0; // Calculated decoded length
        size_t readPosition_ = 0;  // Current read position in decoded data

        int decodeChar(char c);
        bool decodeNextBlock();

    public:
        Base64DecoderStream(std::unique_ptr<IByteSource> underlyingStream, std::ptrdiff_t length, const char *dictionary);
        Base64DecoderStream(const char *data, const char *dictionary);
        Base64DecoderStream(const uint8_t *data, size_t length, const char *dictionary);

        Base64DecoderStream(Base64DecoderStream &&) = default;
        Base64DecoderStream &operator=(Base64DecoderStream &&) = default;
        Base64DecoderStream(const Base64DecoderStream &) = delete;
        Base64DecoderStream &operator=(const Base64DecoderStream &) = delete;

        static Base64DecoderStream create(const char *data, bool isUrlSafe = false);
        static Base64DecoderStream create(const uint8_t *data, size_t length, bool isUrlSafe = false);

        ByteAvailability available() override;
        size_t read(std::span<uint8_t> buffer) override;
        size_t peek(std::span<uint8_t> buffer) override;

    private:
        int peekSingleByte();
        int readSingleByte();
    };

    class Base64EncoderStream : public IByteSource
    {
    private:
        std::unique_ptr<IByteSource> underlyingStream_;
        const char *dictionary_;
        uint8_t buffer_[4];
        int bufferPos_ = 0;
        int bufferSize_ = 0;
        bool eof_ = false;
        bool emitPadding_ = true;

        size_t totalLength_ = 0;
        size_t encodedLength_ = 0;
        size_t readPosition_ = 0;

        bool encodeNextBlock();

    public:
        Base64EncoderStream(std::unique_ptr<IByteSource> underlyingStream, std::ptrdiff_t length, const char *dictionary, bool emitPadding = true);

        Base64EncoderStream(Base64EncoderStream &&) = default;
        Base64EncoderStream &operator=(Base64EncoderStream &&) = default;
        Base64EncoderStream(const Base64EncoderStream &) = delete;
        Base64EncoderStream &operator=(const Base64EncoderStream &) = delete;

        static Base64EncoderStream create(const uint8_t *data, size_t length, bool isUrlSafe = false, bool emitPadding = true);
        static Base64EncoderStream create(const char *data, bool isUrlSafe = false, bool emitPadding = true);

        ByteAvailability available() override;
        size_t read(std::span<uint8_t> buffer) override;
        size_t peek(std::span<uint8_t> buffer) override;

    private:
        int peekSingleByte();
        int readSingleByte();
    };
} // namespace lumalink::http::streams
