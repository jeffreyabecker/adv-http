#pragma once
#include <memory>
#include <functional>

#include "./HttpTimeouts.h"
#include "./IPipelineHandler.h"
#include "./RequestParser.h"
#include "./PipelineHandleClientResult.h"
#include "./NetClient.h"

#include <http_parser.h>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace HttpServerAdvanced
{
    class HttpServerBase;
    class HttpPipeline
    {
    private:
        // HTTP parsing members
        RequestParser requestParser_;

        // Pipeline members
        std::unique_ptr<HttpServerAdvanced::IClient> client_;
        HttpServerBase &server_;
        std::unique_ptr<Stream> responseStream_;
        PipelineHandlerPtr handler_;
        bool completedRequestRead_ = false;
        bool completedResponseWrite_ = false;
        bool haveStartedWritingResponse_ = false;
        bool aborted_ = false;
        bool erroredUnrecoverably_ = false;
        bool timedOutUnrecoverably_ = false;

        uint32_t lastActivityMillis_;
        uint32_t startMillis_;
        uint32_t loopCount_;

        uint8_t _requestBuffer[HttpServerAdvanced::REQUEST_BUFFER_SIZE];
        HttpTimeouts timeouts_;

        enum class PipelineState
        {
            Begin,
            Url,
            HeaderField,
            HeaderValue,
            HeadersComplete,
            Body,
            MessageComplete
        };

        // Internal event handler for HttpRequestParser

        void readFromClientIntoParser()
        {
            if (completedRequestRead_)
            {
                return;
            }
            startActivity();
            int available = 0;
            while ((available = client_->available()) >= 0)
            {

                std::size_t bytesRead = client_->read(_requestBuffer, sizeof(_requestBuffer));

                size_t bytesParsed = requestParser_.execute(_requestBuffer, bytesRead);
                if (bytesParsed < bytesRead)
                {
                    setErroredUnrecoverably();
                    return;
                }
                if (!checkActivityTimeout())
                {
                    return;
                }
            }
            if (available == 0)
            {
                markRequestReadCompleted();
            }
        }

        void writeResponseToClientFromStream()
        {
            if (responseStream_ == nullptr || completedResponseWrite_)
            {
                return;
            }
            startActivity();
            // ClientContext.h uses a 256 byte buffer for copying streams so we will do the same here
            // see: Wifi/src/include/ClientContext.h:379
            uint8_t buff[256];
            int available = 0;
            while ((available = responseStream_->available()) > 0)
            {
                size_t bytesRead = 0;
                for (bytesRead = 0; bytesRead < sizeof(buff) && responseStream_->available(); bytesRead++)
                {
                    int byte = responseStream_->read();
                    if (byte == -1)
                    {
                        break;
                    }
                    buff[bytesRead] = static_cast<uint8_t>(byte);
                }
                if (bytesRead > 0)
                {
                    auto written = client_->write(buff, bytesRead);
                    if (written < bytesRead)
                    {

                        setErroredUnrecoverably();
                        return;
                    }
                }
                else
                {
                    break;
                }
                if (!checkActivityTimeout())
                {
                    return;
                }
            }
            if (available <= 0)
            {
                markResponseWriteCompleted();
            }
        }

        void setupPipeline()
        {
            startMillis_ = millis();
            client_->setTimeout(timeouts_.getActivityTimeout());
        }

        PipelineHandleClientResult _checkStateInHandleClient()
        {
            uint32_t currentMillis = millis();
            if (!client_->connected())
            {
                if (handler_)
                {
                    handler_->onClientDisconnected();
                }
                return PipelineHandleClientResult::ClientDisconnected;
            }

            if (currentMillis - startMillis_ > timeouts_.getTotalRequestLengthMs())
            {
                timedOutUnrecoverably_ = true;
                return PipelineHandleClientResult::TimedOutUnrecoverably;
            }
            if (aborted_)
            {
                return PipelineHandleClientResult::Aborted;
            }
            if (erroredUnrecoverably_)
            {
                return PipelineHandleClientResult::ErroredUnrecoverably;
            }
            if (completedRequestRead_ && completedResponseWrite_)
            {
                return PipelineHandleClientResult::Completed;
            }
            return PipelineHandleClientResult::Processing; // Default case
        }

        bool isFirstLoop() const
        {
            return loopCount_ == 0;
        }

        void startActivity()
        {
            lastActivityMillis_ = millis();
        }

        bool checkActivityTimeout()
        {
            uint32_t currentMillis = millis();
            if (currentMillis - lastActivityMillis_ > timeouts_.getActivityTimeout())
            {
                return false;
            }
            return true;
        }

        void setErroredUnrecoverably()
        {
            erroredUnrecoverably_ = true;
            markRequestReadCompleted();
            markResponseWriteCompleted();
        }

        void markRequestReadCompleted()
        {
            requestParser_.execute(nullptr, 0); // Signal end of input to parser
            completedRequestRead_ = true;
        }

        void markResponseWriteCompleted()
        {
            completedResponseWrite_ = true;
            responseStream_ = nullptr;
        }

    public:
        HttpPipeline(std::unique_ptr<HttpServerAdvanced::IClient> client, HttpServerBase &server,
                     const HttpTimeouts &timeouts, PipelineHandlerPtr parserEventHandler)
            : client_(std::move(client)),
              server_(server),
              timeouts_(timeouts),
              responseStream_(nullptr),
              completedRequestRead_(false),
              completedResponseWrite_(false),
              lastActivityMillis_(0),
              loopCount_(0),
              requestParser_(*handler_),
              handler_(std::move(parserEventHandler))
        {
            handler_->setIPAddress(
                client_->remoteIP(),
                client_->remotePort(),
                client_->localIP(),
                client_->localPort());
            handler_->setResponseStreamCallback([this](std::unique_ptr<Stream> stream)
                                                { this->setResponseStream(std::move(stream)); });
        }

        ~HttpPipeline() = default;

        bool isFinished() const
        {
            return requestParser_.isFinished();
        }

        bool shouldKeepAlive() const
        {
            return requestParser_.shouldKeepAlive();
        }

        void setResponseStream(std::unique_ptr<Stream> responseStream)
        {
            if (haveStartedWritingResponse_)
            {
                assert(false && "Response writing already started");
            }
            responseStream_ = std::move(responseStream);
        }

        PipelineHandleClientResult handleClient()
        {
            loopCount_++;

            if (isFirstLoop())
            {
                setupPipeline();
            }
            auto validationResult = _checkStateInHandleClient();
            if (isPipelineHandleClientResultFinal(validationResult))
            {
                return validationResult;
            }
            readFromClientIntoParser();
            writeResponseToClientFromStream();
            return PipelineHandleClientResult::Processing;
        }

        void abort()
        {
            aborted_ = true;
            abortReadingRequest();
            abortWritingResponse();
        }

        void abortReadingRequest()
        {
            markRequestReadCompleted();
        }

        void abortWritingResponse()
        {
            markResponseWriteCompleted();
        }

        uint32_t startedAt() const
        {
            return startMillis_;
        }

        HttpServerAdvanced::IClient &client()
        {
            return *client_;
        }
    };

} // namespace HttpServerAdvanced
