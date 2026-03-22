#include "UriStream.h"

#include <cstdlib>

namespace HttpServerAdvanced
{
    namespace
    {
        std::vector<std::pair<std::string, std::string>> toOwnedData(std::vector<std::pair<String, String>> &&data)
        {
            std::vector<std::pair<std::string, std::string>> owned;
            owned.reserve(data.size());
            for (auto &pair : data)
            {
                owned.emplace_back(
                    std::string(pair.first.c_str(), pair.first.length()),
                    std::string(pair.second.c_str(), pair.second.length()));
            }
            return owned;
        }

        bool isUriUnreserved(unsigned char byte)
        {
            return (byte >= 'A' && byte <= 'Z') ||
                   (byte >= 'a' && byte <= 'z') ||
                   (byte >= '0' && byte <= '9') ||
                   byte == '-' || byte == '_' || byte == '.' || byte == '~';
        }

        std::string encodeFormComponent(std::string_view value)
        {
            static constexpr char Hex[] = "0123456789ABCDEF";

            std::string encoded;
            encoded.reserve(value.size());
            for (unsigned char ch : value)
            {
                if (isUriUnreserved(ch) || ch == '*')
                {
                    encoded.push_back(static_cast<char>(ch));
                }
                else if (ch == ' ')
                {
                    encoded.push_back('+');
                }
                else
                {
                    encoded.push_back('%');
                    encoded.push_back(Hex[(ch >> 4) & 0x0F]);
                    encoded.push_back(Hex[ch & 0x0F]);
                }
            }

            return encoded;
        }

        std::string buildFormBody(const std::vector<std::pair<std::string, std::string>> &data)
        {
            std::string body;
            bool first = true;
            for (const auto &pair : data)
            {
                if (!first)
                {
                    body.push_back('&');
                }
                first = false;

                body.append(encodeFormComponent(pair.first));
                if (!pair.second.empty())
                {
                    body.push_back('=');
                    body.append(encodeFormComponent(pair.second));
                }
            }

            return body;
        }
    }

    // UriDecodingStream
    UriDecodingStream::UriDecodingStream(const String &uri)
        : UriDecodingStream(std::make_unique<StdStringByteSource>(std::string(uri.c_str(), uri.length())))
    {
    }

    UriDecodingStream::UriDecodingStream(const char *uri)
        : UriDecodingStream(std::make_unique<SpanByteSource>(reinterpret_cast<const uint8_t *>(uri), strlen(uri)))
    {
    }

    UriDecodingStream::UriDecodingStream(const uint8_t *uri, size_t length)
        : UriDecodingStream(std::make_unique<SpanByteSource>(uri, length))
    {
    }

    UriDecodingStream::UriDecodingStream(std::unique_ptr<IByteSource> innerStream)
        : innerStream_(std::move(innerStream))
    {
    }

    AvailableResult UriDecodingStream::available()
    {
        if (!innerStream_)
        {
            return ExhaustedResult();
        }

        const AvailableResult innerAvailable = innerStream_->available();
        if (state_ != State::Normal && innerAvailable.hasBytes())
        {
            return AvailableBytes(1);
        }

        return innerAvailable;
    }

    int UriDecodingStream::readSingleByte()
    {
        int byte = peekSingleByte();
        if (byte != -1)
        {
            if (state_ == State::Normal)
            {
                ReadByte(*innerStream_); // Consume normal character
            }
            // For Percent1 and Percent2 states, the bytes were already consumed in peek()
        }
        return byte;
    }

    int UriDecodingStream::peekSingleByte()
    {
        while (true)
        {
            int byte = PeekByte(*innerStream_);
            if (byte == -1)
            {
                return -1; // End of stream
            }

            if (state_ == State::Normal)
            {
                if (byte == '%')
                {
                    state_ = State::Percent1;
                    ReadByte(*innerStream_); // Consume '%'
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
                ReadByte(*innerStream_); // Consume first hex digit
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
        : UriEncodingStream(std::make_unique<StdStringByteSource>(std::string(uri.c_str(), uri.length())))
    {
    }

    UriEncodingStream::UriEncodingStream(const char *uri)
        : UriEncodingStream(std::make_unique<SpanByteSource>(reinterpret_cast<const uint8_t *>(uri), strlen(uri)))
    {
    }

    UriEncodingStream::UriEncodingStream(const uint8_t *uri, size_t length)
        : UriEncodingStream(std::make_unique<SpanByteSource>(uri, length))
    {
    }

    UriEncodingStream::UriEncodingStream(std::unique_ptr<IByteSource> innerStream)
        : innerStream_(std::move(innerStream))
    {
    }

    AvailableResult UriEncodingStream::available()
    {
        if (state_ != State::Normal)
        {
            return AvailableBytes(1);
        }

        return innerStream_ ? innerStream_->available() : ExhaustedResult();
    }

    int UriEncodingStream::readSingleByte()
    {
        int byte = peekSingleByte();
        if (byte != -1)
        {
            encodedIndex_++;
            if (encodedIndex_ >= 3 || state_ == State::Normal)
            {
                encodedIndex_ = 0;
                state_ = State::Normal;
                ReadByte(*innerStream_);
            }
        }
        return byte;
    }

    int UriEncodingStream::peekSingleByte()
    {
        while (true)
        {
            if (state_ == State::Normal)
            {
                int byte = PeekByte(*innerStream_);
                if (byte == -1)
                {
                    return -1;
                }

                // Check if byte needs encoding (unreserved characters per RFC 3986)
                if (isUriUnreserved(static_cast<unsigned char>(byte)))
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

    FormEncodingStream::FormEncodingStream(std::vector<std::pair<std::string, std::string>> &&data)
        : StdStringByteSource(buildFormBody(data))
    {
    }

    FormEncodingStream::FormEncodingStream(std::vector<std::pair<String, String>> &&data)
        : FormEncodingStream(toOwnedData(std::move(data)))
    {
    }
}
