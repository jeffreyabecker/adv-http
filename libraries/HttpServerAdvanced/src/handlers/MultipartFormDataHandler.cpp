#include "./MultipartFormDataHandler.h"
#include <cstring>

namespace HttpServerAdvanced
{
    void MultipartFormDataHandler::extractBoundary(const String &contentType)
    {
        const char *str = contentType.c_str();
        const char *boundaryPos = strstr(str, "boundary=");
        if (boundaryPos)
        {
            boundaryPos += 9; // strlen("boundary=")
            const char *endPos = strchr(boundaryPos, ';');
            if (!endPos)
                endPos = boundaryPos + strlen(boundaryPos);
            boundary_ = String(boundaryPos, endPos - boundaryPos);
            boundary_.trim();
        }
    }

    void MultipartFormDataHandler::parsePartHeaders()
    {
        const char *headerEnd = strstr((const char *)buffer_, "\r\n\r\n");
        if (!headerEnd)
            return; // Not enough data yet

        String headerStr((const char *)buffer_, headerEnd - (const char *)buffer_);

        // Parse Content-Disposition header
        int dispPos = headerStr.indexOf("Content-Disposition:");
        if (dispPos >= 0)
        {
            int nameStart = headerStr.indexOf("name=\"", dispPos);
            if (nameStart >= 0)
            {
                nameStart += 6;
                int nameEnd = headerStr.indexOf("\"", nameStart);
                if (nameEnd > nameStart)
                {
                    partName_ = headerStr.substring(nameStart, nameEnd);
                }
            }

            int filenameStart = headerStr.indexOf("filename=\"", dispPos);
            if (filenameStart >= 0)
            {
                filenameStart += 10;
                int filenameEnd = headerStr.indexOf("\"", filenameStart);
                if (filenameEnd > filenameStart)
                {
                    filename_ = headerStr.substring(filenameStart, filenameEnd);
                }
            }
        }

        // Parse Content-Type header
        int typePos = headerStr.indexOf("Content-Type:");
        if (typePos >= 0)
        {
            int typeStart = typePos + 13; // strlen("Content-Type:")
            while (typeStart < (int)headerStr.length() && headerStr[typeStart] == ' ')
                typeStart++;
            int typeEnd = headerStr.indexOf("\r\n", typeStart);
            if (typeEnd < 0)
                typeEnd = headerStr.length();
            contentType_ = headerStr.substring(typeStart, typeEnd);
        }
        else
        {
            contentType_ = ""; // Empty if not specified
        }

        parsingHeaders_ = false;
        size_t headerSize = (headerEnd - (const char *)buffer_) + 4; // +4 for \r\n\r\n
        memmove(buffer_, buffer_ + headerSize, bufferLength_ - headerSize);
        bufferLength_ -= headerSize;
    }

    const uint8_t *MultipartFormDataHandler::findBoundary(const uint8_t *start, size_t len)
    {
        const char *boundaryStr = boundary_.c_str();
        size_t boundaryLen = boundary_.length();
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
} // namespace HttpServerAdvanced
