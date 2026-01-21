#pragma once
#include <Arduino.h>
#include <vector>
#include <cstring>
#include <optional>
#include "./IHttpHandler.h"
#include "./HttpResponse.h"
#include "./HttpHeader.h"
#include "./Buffer.h"
#include "./Defines.h"

namespace HttpServerAdvanced
{
    enum class MultipartStatus
    {
        FirstChunk,       // First chunk of a multipart form field
        SubsequentChunk,  // Middle chunks of a multipart form field
        FinalChunk,       // Last chunk of a multipart form field (boundary found)
        Completed         // All multipart data processed
    };

    class MultipartFormDataBuffer : public Buffer
    {
    private:
        const String &filename_;
        const String &contentType_;
        const String &partName_;
        MultipartStatus status_ = MultipartStatus::FirstChunk;

    public:
        MultipartFormDataBuffer(const uint8_t *data, size_t size, const String &filename, const String &contentType, const String &name, MultipartStatus status)
            : Buffer(data, size), filename_(filename), contentType_(contentType), partName_(name), status_(status) {}
        const String &filename() const { return filename_; }
        const String &contentType() const { return contentType_; }
        const String &name() const { return partName_; }
        MultipartStatus status() const { return status_; }
    };

    class MultipartFormDataHandler : public IHttpHandler
    {

    private:
        std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &, MultipartFormDataBuffer)> handler_;
        ParameterExtractor extractor_;
        HandlerResult response_;
        std::vector<String> params_;
        String boundary_;
        String filename_;
        String contentType_;
        String partName_;
        uint8_t buffer_[MULTIPART_FORM_DATA_BUFFER_SIZE];
        size_t bufferLength_ = 0;
        bool parsingHeaders_ = true;
        bool partStarted_ = false;  // Track if we've emitted first chunk of current part
        MultipartStatus status_ = MultipartStatus::FirstChunk;
        std::vector<uint8_t> partData_;

        // Extract boundary from Content-Type header
        void extractBoundary(const String &contentType)
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

        // Parse part headers (Content-Disposition, Content-Type) from buffer
        void parsePartHeaders()
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

        // Find boundary in buffer
        const uint8_t *findBoundary(const uint8_t *start, size_t len)
        {
            const char *boundaryStr = boundary_.c_str();
            size_t boundaryLen = boundary_.length();
            for (size_t i = 0; i + boundaryLen + 2 <= len; ++i)
            {
                if (start[i] == '\r' && start[i+1] == '\n' && 
                    memcmp(start + i + 2, boundaryStr, boundaryLen) == 0)
                {
                    return start + i;
                }
            }
            return nullptr;
        }

    public:
        MultipartFormDataHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &, MultipartFormDataBuffer)> handler, ParameterExtractor extractor)
            : handler_(handler), extractor_(extractor) {}
        virtual HandlerResult handleStep(HttpRequest &context)
        {
            if (!response_ && context.completedPhases() >= HttpRequestPhase::CompletedReadingMessage)
            {
                handleBodyChunk(context, nullptr, 0);
            }
            if (response_)
            {
                return std::move(response_);
            }
            return nullptr;
        }
        virtual void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length)
        {
            if (response_)
            {
                return;
            }
            
            // Extract boundary on first chunk
            if (boundary_.length() == 0)
            {
                params_ = extractor_(context);
                std::optional<HttpHeader> contentTypeHeader = context.headers().find(HttpHeader::ContentType);
                if (contentTypeHeader.has_value())
                {
                    extractBoundary(contentTypeHeader->value());
                }
                if (boundary_.length() == 0)
                {
                    response_ = HttpResponse::create(HttpStatus::BadRequest(), "Missing or invalid boundary");
                    return;
                }
            }
            
            // Append data to buffer
            if (at && length > 0)
            {
                if (bufferLength_ + length > sizeof(buffer_))
                {
                    length = sizeof(buffer_) - bufferLength_;
                }
                memcpy(buffer_ + bufferLength_, at, length);
                bufferLength_ += length;
            }
            
            // Parse part headers if needed
            if (parsingHeaders_ && bufferLength_ > 4)
            {
                parsePartHeaders();
            }
            
            // Look for boundary
            if (!parsingHeaders_)
            {
                const uint8_t *boundaryPos = findBoundary(buffer_, bufferLength_);
                if (boundaryPos)
                {
                    // Extract part data before boundary (final chunk)
                    size_t dataLen = boundaryPos - buffer_;
                    if (dataLen > 0)
                    {
                        partData_.insert(partData_.end(), buffer_, buffer_ + dataLen);
                    }
                    
                    // Invoke handler with accumulated part data marked as final
                    if (partName_.length() > 0)
                    {
                        MultipartStatus finalStatus = partStarted_ ? MultipartStatus::FinalChunk : MultipartStatus::FirstChunk;
                        response_ = handler_(context, params_, 
                            MultipartFormDataBuffer(partData_.data(), partData_.size(), 
                                                   filename_, contentType_, partName_, finalStatus));
                    }
                    
                    // Reset for next part
                    partStarted_ = false;
                    partData_.clear();
                    filename_ = "";
                    contentType_ = "";
                    partName_ = "";
                    parsingHeaders_ = true;
                    
                    // Move to next part
                    size_t boundaryLen = boundary_.length() + 4; // +4 for \r\n and initial \r\n
                    if (boundaryPos - buffer_ + boundaryLen < bufferLength_)
                    {
                        memmove(buffer_, boundaryPos + boundaryLen, bufferLength_ - (boundaryPos - buffer_ + boundaryLen));
                        bufferLength_ -= (boundaryPos - buffer_ + boundaryLen);
                    }
                    else
                    {
                        bufferLength_ = 0;
                    }
                }
                else if (bufferLength_ > boundary_.length() + 4)
                {
                    // Keep boundary-sized tail; flush excess as part data via handler
                    // This enables streaming of large files
                    size_t flushLen = bufferLength_ - (boundary_.length() + 4);
                    
                    if (partName_.length() > 0 && flushLen > 0)
                    {
                        partData_.insert(partData_.end(), buffer_, buffer_ + flushLen);
                        
                        // Invoke handler with accumulated data to stream it
                        MultipartStatus chunkStatus = partStarted_ ? MultipartStatus::SubsequentChunk : MultipartStatus::FirstChunk;
                        response_ = handler_(context, params_, 
                            MultipartFormDataBuffer(partData_.data(), partData_.size(), 
                                                   filename_, contentType_, partName_, chunkStatus));
                        
                        if (!response_)
                        {
                            partStarted_ = true;  // Mark that we've emitted at least one chunk
                            partData_.clear();
                        }
                    }
                    
                    memmove(buffer_, buffer_ + flushLen, boundary_.length() + 4);
                    bufferLength_ = boundary_.length() + 4;
                }
            }
        }
    };

} // namespace HttpServerAdvanced
