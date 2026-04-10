#include "Base64Stream.h"
#include <span>
#include <cstring>

namespace lumalink::http::streams
{
    // Base64DecoderStream

    int Base64DecoderStream::decodeChar(char c)
    {
        if (c == '=')
            return -1;
        const char *pos = strchr(dictionary_, c);
        return pos ? (pos - dictionary_) : -2;
    }

    bool Base64DecoderStream::decodeNextBlock()
    {
        // Read 4 base64 characters
        int b[4];
        for (int i = 0; i < 4; i++)
        {
            int c = underlyingStream_ ? ReadByte(*underlyingStream_) : -1;
            if (c == -1)
            {
                eof_ = true;
                return false;
            }
            b[i] = decodeChar(c);
            if (b[i] == -2)
                return false; // Invalid character
        }

        // Decode 3 bytes from 4 base64 chars
        buffer_[0] = (b[0] << 2) | (b[1] >> 4);
        buffer_[1] = ((b[1] & 0x0F) << 4) | (b[2] >> 2);
        buffer_[2] = ((b[2] & 0x03) << 6) | b[3];

        bufferSize_ = 3;
        if (b[2] == -1)
            bufferSize_ = 1;
        else if (b[3] == -1)
            bufferSize_ = 2;

        bufferPos_ = 0;
        return true;
    }

    Base64DecoderStream::Base64DecoderStream(std::unique_ptr<IByteSource> underlyingStream, std::ptrdiff_t length, const char *dictionary)
        : underlyingStream_(std::move(underlyingStream)), dictionary_(dictionary), totalLength_(length > 0 ? length : 0),
          decodedLength_(length > 0 ? (length / 4) * 3 : 0)
    {
    }

    Base64DecoderStream::Base64DecoderStream(const char *data, const char *dictionary)
        : Base64DecoderStream(std::make_unique<lumalink::platform::buffers::SpanByteSource>(reinterpret_cast<const uint8_t *>(data), strlen(data)), strlen(data), dictionary)
    {
    }

    Base64DecoderStream::Base64DecoderStream(const uint8_t *data, size_t length, const char *dictionary)
        : Base64DecoderStream(std::make_unique<lumalink::platform::buffers::SpanByteSource>(data, length), length, dictionary)
    {
    }

    Base64DecoderStream Base64DecoderStream::create(const char *data, bool isUrlSafe)
    {
        return Base64DecoderStream(data, isUrlSafe ? base64_url_chars : base64_chars);
    }

    Base64DecoderStream Base64DecoderStream::create(const uint8_t *data, size_t length, bool isUrlSafe)
    {
        return Base64DecoderStream(data, length, isUrlSafe ? base64_url_chars : base64_chars);
    }

    AvailableResult Base64DecoderStream::available()
    {
        if (bufferPos_ < bufferSize_)
        {
            return lumalink::platform::buffers::AvailableBytes(static_cast<size_t>(bufferSize_ - bufferPos_));
        }

        if (totalLength_ > 0)
        {
            const size_t remaining = decodedLength_ > readPosition_ ? (decodedLength_ - readPosition_) : 0;
            return remaining > 0 ? lumalink::platform::buffers::AvailableBytes(remaining) : lumalink::platform::buffers::ExhaustedResult();
        }

        if (!underlyingStream_)
        {
            return lumalink::platform::buffers::ExhaustedResult();
        }

        const AvailableResult underlyingAvailable = underlyingStream_->available();
        if (underlyingAvailable.hasBytes())
        {
            return lumalink::platform::buffers::AvailableBytes((underlyingAvailable.count / 4) * 3);
        }

        return underlyingAvailable;
    }

    size_t Base64DecoderStream::read(std::span<uint8_t> buffer)
    {
        size_t totalRead = 0;
        while (totalRead < buffer.size())
        {
            const int value = readSingleByte();
            if (value < 0)
            {
                break;
            }

            buffer[totalRead++] = static_cast<uint8_t>(value);
        }

        return totalRead;
    }

    size_t Base64DecoderStream::peek(std::span<uint8_t> buffer)
    {
        if (buffer.empty())
        {
            return 0;
        }

        const int value = peekSingleByte();
        if (value < 0)
        {
            return 0;
        }

        buffer[0] = static_cast<uint8_t>(value);
        return 1;
    }

    int Base64DecoderStream::peekSingleByte()
    {
        if (bufferPos_ < bufferSize_)
        {
            return buffer_[bufferPos_];
        }

        if (eof_)
            return -1;

        if (!decodeNextBlock())
            return -1;

        return buffer_[bufferPos_];
    }

    int Base64DecoderStream::readSingleByte()
    {
        int val = peekSingleByte();
        if (val != -1)
        {
            bufferPos_++;
            readPosition_++;
        }
        return val;
    }

    // Base64EncoderStream

    bool Base64EncoderStream::encodeNextBlock()
    {
        uint8_t input[3] = {0, 0, 0};
        int inputSize = 0;

        for (int i = 0; i < 3; i++)
        {
            int c = underlyingStream_ ? ReadByte(*underlyingStream_) : -1;
            if (c == -1)
            {
                eof_ = true;
                break;
            }
            input[i] = c;
            inputSize++;
        }

        if (inputSize == 0)
            return false;

        // Encode 3 bytes to 4 base64 characters
        buffer_[0] = dictionary_[input[0] >> 2];
        buffer_[1] = dictionary_[((input[0] & 0x03) << 4) | (input[1] >> 4)];
        buffer_[2] = inputSize > 1 ? dictionary_[((input[1] & 0x0F) << 2) | (input[2] >> 6)] : (emitPadding_ ? '=' : -1);
        buffer_[3] = inputSize > 2 ? dictionary_[input[2] & 0x3F] : (emitPadding_ ? '=' : -1);

        bufferSize_ = emitPadding_ ? 4 : inputSize + (inputSize == 1 ? 1 : 0);
        bufferPos_ = 0;
        return true;
    }

    Base64EncoderStream::Base64EncoderStream(std::unique_ptr<IByteSource> underlyingStream, std::ptrdiff_t length, const char *dictionary, bool emitPadding)
        : underlyingStream_(std::move(underlyingStream)), dictionary_(dictionary), emitPadding_(emitPadding), totalLength_(length > 0 ? length : 0),
          encodedLength_(length > 0 ? ((length + 2) / 3) * (emitPadding ? 4 : 3) : 0)
    {
    }

    Base64EncoderStream Base64EncoderStream::create(const uint8_t *data, size_t length, bool isUrlSafe, bool emitPadding)
    {
        return Base64EncoderStream(std::make_unique<lumalink::platform::buffers::SpanByteSource>(data, length), length, isUrlSafe ? base64_url_chars : base64_chars, !isUrlSafe && emitPadding);
    }

    Base64EncoderStream Base64EncoderStream::create(const char *data, bool isUrlSafe, bool emitPadding)
    {
        return Base64EncoderStream(std::make_unique<lumalink::platform::buffers::SpanByteSource>(reinterpret_cast<const uint8_t *>(data), strlen(data)), strlen(data), isUrlSafe ? base64_url_chars : base64_chars, !isUrlSafe && emitPadding);
    }

    AvailableResult Base64EncoderStream::available()
    {
        if (bufferPos_ < bufferSize_)
        {
            return lumalink::platform::buffers::AvailableBytes(static_cast<size_t>(bufferSize_ - bufferPos_));
        }

        if (totalLength_ > 0)
        {
            const size_t remaining = encodedLength_ > readPosition_ ? (encodedLength_ - readPosition_) : 0;
            return remaining > 0 ? lumalink::platform::buffers::AvailableBytes(remaining) : lumalink::platform::buffers::ExhaustedResult();
        }

        if (!underlyingStream_)
        {
            return lumalink::platform::buffers::ExhaustedResult();
        }

        const AvailableResult underlyingAvailable = underlyingStream_->available();
        if (underlyingAvailable.hasBytes())
        {
            return lumalink::platform::buffers::AvailableBytes(1);
        }

        return underlyingAvailable;
    }

    size_t Base64EncoderStream::read(std::span<uint8_t> buffer)
    {
        size_t totalRead = 0;
        while (totalRead < buffer.size())
        {
            const int value = readSingleByte();
            if (value < 0)
            {
                break;
            }

            buffer[totalRead++] = static_cast<uint8_t>(value);
        }

        return totalRead;
    }

    size_t Base64EncoderStream::peek(std::span<uint8_t> buffer)
    {
        if (buffer.empty())
        {
            return 0;
        }

        const int value = peekSingleByte();
        if (value < 0)
        {
            return 0;
        }

        buffer[0] = static_cast<uint8_t>(value);
        return 1;
    }

    int Base64EncoderStream::peekSingleByte()
    {
        if (bufferPos_ < bufferSize_)
            return buffer_[bufferPos_];

        if (eof_ && bufferSize_ == 0)
            return -1;

        if (!encodeNextBlock())
            return -1;

        return buffer_[bufferPos_];
    }

    int Base64EncoderStream::readSingleByte()
    {
        int val = peekSingleByte();
        if (val != -1)
        {
            bufferPos_++;
            readPosition_++;
        }
        return val;
    }

} // namespace lumalink::http::streams
