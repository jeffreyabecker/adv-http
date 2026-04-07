#include "MultipartFormDataHandler.h"
#include <cstring>

namespace lumalink::http::handlers
{
    namespace
    {
        std::string_view trimAscii(std::string_view value)
        {
            const auto start = value.find_first_not_of(" \t\r\n");
            if (start == std::string_view::npos)
            {
                return std::string_view();
            }

            const auto end = value.find_last_not_of(" \t\r\n");
            return value.substr(start, end - start + 1);
        }
    }

    void MultipartFormDataHandler::extractBoundary(std::string_view contentType)
    {
        const std::size_t boundaryPos = contentType.find("boundary=");
        if (boundaryPos != std::string_view::npos)
        {
            const std::size_t valueStart = boundaryPos + 9;
            const std::size_t valueEnd = contentType.find(';', valueStart);
            const std::string_view rawBoundary = valueEnd == std::string_view::npos
                                                     ? contentType.substr(valueStart)
                                                     : contentType.substr(valueStart, valueEnd - valueStart);
            const std::string_view trimmedBoundary = trimAscii(rawBoundary);
            boundary_.assign(trimmedBoundary.data(), trimmedBoundary.size());
        }
    }

    void MultipartFormDataHandler::parsePartHeaders()
    {
        static constexpr uint8_t HeaderTerminator[] = {'\r', '\n', '\r', '\n'};
        const uint8_t *bufferBegin = buffer_;
        const uint8_t *bufferEnd = buffer_ + bufferLength_;
        const uint8_t *headerEnd = std::search(bufferBegin, bufferEnd, HeaderTerminator, HeaderTerminator + 4);
        if (headerEnd == bufferEnd)
            return; // Not enough data yet

        const std::string_view headerStr(reinterpret_cast<const char *>(buffer_), static_cast<std::size_t>(headerEnd - buffer_));
        filename_.clear();
        contentType_.clear();
        partName_.clear();

        // Parse Content-Disposition header
        const std::size_t dispPos = headerStr.find("Content-Disposition:");
        if (dispPos != std::string_view::npos)
        {
            const std::size_t nameStartMarker = headerStr.find("name=\"", dispPos);
            if (nameStartMarker != std::string_view::npos)
            {
                const std::size_t nameStart = nameStartMarker + 6;
                const std::size_t nameEnd = headerStr.find('"', nameStart);
                if (nameEnd != std::string_view::npos && nameEnd > nameStart)
                {
                    partName_.assign(headerStr.data() + nameStart, nameEnd - nameStart);
                }
            }

            const std::size_t filenameStartMarker = headerStr.find("filename=\"", dispPos);
            if (filenameStartMarker != std::string_view::npos)
            {
                const std::size_t filenameStart = filenameStartMarker + 10;
                const std::size_t filenameEnd = headerStr.find('"', filenameStart);
                if (filenameEnd != std::string_view::npos && filenameEnd > filenameStart)
                {
                    filename_.assign(headerStr.data() + filenameStart, filenameEnd - filenameStart);
                }
            }
        }

        // Parse Content-Type header
        const std::size_t typePos = headerStr.find("Content-Type:");
        if (typePos != std::string_view::npos)
        {
            std::size_t typeStart = typePos + 13; // strlen("Content-Type:")
            while (typeStart < headerStr.size() && headerStr[typeStart] == ' ')
                typeStart++;
            std::size_t typeEnd = headerStr.find("\r\n", typeStart);
            if (typeEnd == std::string_view::npos)
                typeEnd = headerStr.size();
            contentType_.assign(headerStr.data() + typeStart, typeEnd - typeStart);
        }

        parsingHeaders_ = false;
        size_t headerSize = static_cast<size_t>(headerEnd - buffer_) + 4; // +4 for \r\n\r\n
        memmove(buffer_, buffer_ + headerSize, bufferLength_ - headerSize);
        bufferLength_ -= headerSize;
    }

    const uint8_t *MultipartFormDataHandler::findBoundary(const uint8_t *start, size_t len)
    {
        const char *boundaryStr = boundary_.data();
        size_t boundaryLen = boundary_.size();
        for (size_t i = 0; i + boundaryLen + 2 <= len; ++i)
        {
            if (start[i] == '\r' && start[i + 1] == '\n' &&
                memcmp(start + i + 2, boundaryStr, boundaryLen) == 0)
            {
                return start + i;
            }
        }
        return nullptr;
    }
} // namespace lumalink::http::handlers
