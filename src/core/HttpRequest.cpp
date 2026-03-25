#include "../core/HttpRequest.h"
#include "HttpRequestPipelineAdapter.h"
#include "HttpRequestRunner.h"
#include "../handlers/IHttpHandler.h"
#include "IHttpRequestHandlerFactory.h"

namespace HttpServerAdvanced
{
    namespace
    {
        class DefaultHttpRequestRunner : public HttpRequestRunner
        {
        public:
            DefaultHttpRequestRunner(HttpServerBase &server, IHttpRequestHandlerFactory &handlerFactory)
                : context_(server, handlerFactory)
            {
                context_.bindCompletedPhases(&completedPhases_);
            }

            HttpRequest &context() override
            {
                return context_;
            }

            int onMessageBegin(const char *method,
                               std::uint16_t versionMajor,
                               std::uint16_t versionMinor,
                               std::string_view url) override
            {
                context_.setRequestLine(method, versionMajor, versionMinor, url);
                return 0;
            }

            void setAddresses(std::string_view remoteAddress,
                              std::uint16_t remotePort,
                              std::string_view localAddress,
                              std::uint16_t localPort) override
            {
                context_.setRequestAddresses(remoteAddress, remotePort, localAddress, localPort);
            }

            int onHeader(std::string_view field, std::string_view value) override
            {
                context_.setHeader(field, value);
                return 0;
            }

            int onHeadersComplete() override
            {
                return 0;
            }

            int onBody(const std::uint8_t *at, std::size_t length) override
            {
                IHttpHandler *handler = tryGetHandler();
                if (bodyBytesReceived_ == 0)
                {
                    advance(HttpRequestPhase::BeginReadingBody);
                }

                if (handler != nullptr)
                {
                    handler->handleBodyChunk(context_, at, length);
                }

                bodyBytesReceived_ += length;
                handleStep();
                return 0;
            }

            void advance(HttpRequestPhaseFlags trigger) override
            {
                completedPhases_ |= trigger;
                handleStep();
            }

            void onError(PipelineError error) override
            {
                if (pendingResult_.hasValue())
                {
                    return;
                }

                HttpStatus status;
                std::string message;

                switch (error.code())
                {
                case HttpServerAdvanced::PipelineErrorCode::InvalidVersion:
                case HttpServerAdvanced::PipelineErrorCode::InvalidMethod:
                    status = HttpStatus::BadRequest();
                    message = "Bad Request: ";
                    break;
                case HttpServerAdvanced::PipelineErrorCode::UriTooLong:
                case HttpServerAdvanced::PipelineErrorCode::HeaderTooLarge:
                case HttpServerAdvanced::PipelineErrorCode::BodyTooLarge:
                    status = HttpStatus::PayloadTooLarge();
                    message = "Payload Too Large: ";
                    break;
                case HttpServerAdvanced::PipelineErrorCode::Timeout:
                    status = HttpStatus::RequestTimeout();
                    message = "Request Timeout: ";
                    break;
                case HttpServerAdvanced::PipelineErrorCode::UnsupportedMediaType:
                    status = HttpStatus::UnsupportedMediaType();
                    message = "Unsupported Media Type: ";
                    break;
                default:
                    status = HttpStatus::BadRequest();
                    message = "Bad Request: ";
                    break;
                }

                std::unique_ptr<IHttpResponse> response = context_.createResponse(status, message + std::string(error.message()));
                setPendingResult(RequestHandlingResult::response(CreateResponseStream(std::move(response))));
                markResponseStarted();
            }

            void onClientDisconnected() override
            {
            }

            bool hasPendingResult() const override
            {
                return pendingResult_.hasValue();
            }

            RequestHandlingResult takeResult() override
            {
                RequestHandlingResult result = std::move(pendingResult_);
                pendingResult_ = RequestHandlingResult();
                return result;
            }

        private:
            HttpRequest context_;
            std::unique_ptr<IHttpHandler> handler_;
            RequestHandlingResult pendingResult_;
            std::size_t bodyBytesReceived_ = 0;
            HttpRequestPhaseFlags completedPhases_ = 0;

            IHttpHandler *tryGetHandler()
            {
                if (!handler_)
                {
                    handler_ = context_.createHandler();
                }

                return handler_.get();
            }

            void setPendingResult(RequestHandlingResult result)
            {
                pendingResult_ = std::move(result);
            }

            void markResponseStarted()
            {
                completedPhases_ |= HttpRequestPhase::WritingResponseStarted;
            }

            void handleStep()
            {
                if (pendingResult_.hasValue())
                {
                    return;
                }

                IHttpHandler *handler = tryGetHandler();
                if (handler != nullptr)
                {
                    HandlerResult handlerResult = handler->handleStep(context_);
                    if (handlerResult.hasValue())
                    {
                        switch (handlerResult.kind)
                        {
                        case HandlerResult::Kind::Response:
                            setPendingResult(RequestHandlingResult::response(CreateResponseStream(std::move(handlerResult.response))));
                            markResponseStarted();
                            break;
                        case HandlerResult::Kind::Upgrade:
                            setPendingResult(RequestHandlingResult::upgrade(std::move(handlerResult.upgradedSession)));
                            break;
                        case HandlerResult::Kind::None:
                        default:
                            break;
                        }

                        return;
                    }
                }

                if ((completedPhases_ & HttpRequestPhase::CompletedReadingMessage) != 0 && !pendingResult_.hasValue())
                {
                    setPendingResult(RequestHandlingResult::noResponse());
                }
            }
        };
    }

    PipelineHandlerPtr HttpRequest::createPipelineHandler(HttpServerBase &server, IHttpRequestHandlerFactory &handlerFactory)
    {
        return PipelineHandlerPtr(
            new HttpRequestPipelineAdapter(std::make_unique<DefaultHttpRequestRunner>(server, handlerFactory)),
            [](IPipelineHandler *handler)
            {
                delete handler;
            });
    }
} // namespace HttpServerAdvanced



