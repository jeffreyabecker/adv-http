#include "./UriStream.h"
namespace HttpServerAdvanced
{
    // UriDecodingStream
    UriDecodingStream::UriDecodingStream(const String &uri)
        : UriDecodingStream(std::make_unique<StringStream>(uri))
    {
    }

    UriDecodingStream::UriDecodingStream(const char *uri)
        : UriDecodingStream(std::make_unique<OctetsStream>(uri))
    {
    }

    UriDecodingStream::UriDecodingStream(const uint8_t *uri, size_t length)
        : UriDecodingStream(std::make_unique<OctetsStream>(uri, length))
    {
    }

    UriDecodingStream::UriDecodingStream(std::unique_ptr<ReadStream> innerStream)
        : innerStream_(std::move(innerStream))
    {
    }

    int UriDecodingStream::available()
    {
        return innerStream_->available();
    }

    int UriDecodingStream::read()
    {
        int byte = peek();
        if (byte != -1)
        {
            if (state_ == State::Normal)
            {
                innerStream_->read(); // Consume normal character
            }
            // For Percent1 and Percent2 states, the bytes were already consumed in peek()
        }
        return byte;
    }

    int UriDecodingStream::peek()
    {
        while (true)
        {
            int byte = innerStream_->peek();
            if (byte == -1)
            {
                return -1; // End of stream
            }

            if (state_ == State::Normal)
            {
                if (byte == '%')
                {
                    state_ = State::Percent1;
                    innerStream_->read(); // Consume '%'
                }
                else if (byte == '+')
                {
                    return ' ';
                }
                else
                {
                    return byte; // Normal character
                }
            }
            else if (state_ == State::Percent1)
            {
                percentBuffer_[0] = static_cast<char>(byte);
                state_ = State::Percent2;
                innerStream_->read(); // Consume first hex digit
            }
            else if (state_ == State::Percent2)
            {
                percentBuffer_[1] = static_cast<char>(byte);
                // Decode the percent-encoded byte
                char hex[3] = {percentBuffer_[0], percentBuffer_[1], '\0'};
                char decoded = static_cast<char>(strtol(hex, nullptr, 16));

                // Reset state
                state_ = State::Normal;
                percentIndex_ = 0;

                return decoded;
            }
        }
    }

    // UriEncodingStream
    UriEncodingStream::UriEncodingStream(const String &uri)
        : UriEncodingStream(std::make_unique<StringStream>(uri))
    {
    }

    UriEncodingStream::UriEncodingStream(const char *uri)
        : UriEncodingStream(std::make_unique<OctetsStream>(uri))
    {
    }

    UriEncodingStream::UriEncodingStream(const uint8_t *uri, size_t length)
        : UriEncodingStream(std::make_unique<OctetsStream>(uri, length))
    {
    }

    UriEncodingStream::UriEncodingStream(std::unique_ptr<ReadStream> innerStream)
        : innerStream_(std::move(innerStream))
    {
    }

    int UriEncodingStream::available()
    {
        return innerStream_->available();
    }

    int UriEncodingStream::read()
    {
        int byte = peek();
        if (byte != -1)
        {
            encodedIndex_++;
            if (encodedIndex_ >= 3 || state_ == State::Normal)
            {
                encodedIndex_ = 0;
                state_ = State::Normal;
                innerStream_->read();
            }
        }
        return byte;
    }

    int UriEncodingStream::peek()
    {
        while (true)
        {
            if (state_ == State::Normal)
            {
                int byte = innerStream_->peek();
                if (byte == -1)
                {
                    return -1;
                }

                // Check if byte needs encoding (unreserved characters per RFC 3986)
                if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
                    (byte >= '0' && byte <= '9') || byte == '-' || byte == '_' ||
                    byte == '.' || byte == '~')
                {
                    return byte; // Safe character, return as-is
                }
                else
                {
                    // Encode this byte
                    snprintf(encodedBuffer_, sizeof(encodedBuffer_), "%02X", byte);
                    state_ = State::EncodedPercent;
                    encodedIndex_ = 0;
                    return '%';
                }
            }
            else if (state_ == State::EncodedPercent)
            {
                state_ = State::EncodedHex1;
                return encodedBuffer_[0];
            }
            else if (state_ == State::EncodedHex1)
            {
                state_ = State::Normal;
                return encodedBuffer_[1];
            }
        }
    }
}
