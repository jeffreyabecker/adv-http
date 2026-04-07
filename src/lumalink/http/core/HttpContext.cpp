#include "../core/HttpContext.h"
#include "HttpContextPipelineAdapter.h"
#include "HttpContextRunner.h"
#include "../handlers/IHttpHandler.h"
#include "../response/StringResponse.h"
#include "IHttpContextHandlerFactory.h"

namespace lumalink::http::core
{
    class HttpContextAccess
    {
    public:
        static void bindCompletedPhases(HttpContext &request, const HttpContextPhaseFlags *completedPhases)
        {
            request.bindCompletedPhases(completedPhases);
        }

        static void setRequestLine(HttpContext &request,
                                   const char *method,
                                   std::uint16_t versionMajor,
                                   std::uint16_t versionMinor,
                                   std::string_view url)
        {
            request.setRequestLine(method, versionMajor, versionMinor, url);
        }

        static void setRequestAddresses(HttpContext &request,
                                        std::string_view remoteAddress,
                                        std::uint16_t remotePort,
                                        std::string_view localAddress,
                                        std::uint16_t localPort)
        {
            request.setRequestAddresses(remoteAddress, remotePort, localAddress, localPort);
        }

        static void setHeader(HttpContext &request, std::string_view field, std::string_view value)
        {
            request.setHeader(field, value);
        }

        static std::unique_ptr<lumalink::http::handlers::IHttpHandler> createHandler(HttpContext &request)
        {
            return request.createHandler();
        }
    };

    namespace
    {
        class DefaultHttpContextRunner : public HttpContextRunner
        {
        public:
            DefaultHttpContextRunner(lumalink::http::server::HttpServerBase &server, IHttpContextHandlerFactory &handlerFactory)
                : context_(server, handlerFactory)
            {
                HttpContextAccess::bindCompletedPhases(context_, &completedPhases_);
            }

            lumalink::http::core::HttpRequestContext &context() override
            {
                return context_;
            }

            int onMessageBegin(const char *method,
                               std::uint16_t versionMajor,
                               std::uint16_t versionMinor,
                               std::string_view url) override
            {
                HttpContextAccess::setRequestLine(context_, method, versionMajor, versionMinor, url);
                return 0;
            }

            void setAddresses(std::string_view remoteAddress,
                              std::uint16_t remotePort,
                              std::string_view localAddress,
                              std::uint16_t localPort) override
            {
                HttpContextAccess::setRequestAddresses(context_, remoteAddress, remotePort, localAddress, localPort);
            }

            int onHeader(std::string_view field, std::string_view value) override
            {
                HttpContextAccess::setHeader(context_, field, value);
                return 0;
            }

            int onHeadersComplete() override
            {
                return 0;
            }

            int onBody(const std::uint8_t *at, std::size_t length) override
            {
                lumalink::http::handlers::IHttpHandler *handler = tryGetHandler();
                if (bodyBytesReceived_ == 0)
                {
                    advance(HttpContextPhase::BeginReadingBody);
                }

                if (handler != nullptr)
                {
                    handler->handleBodyChunk(context_, at, length);
                }

                bodyBytesReceived_ += length;
                handleStep();
                return 0;
            }

            void advance(HttpContextPhaseFlags trigger) override
            {
                completedPhases_ |= trigger;
                handleStep();
            }

            void onError(lumalink::http::pipeline::PipelineError error) override
            {
                if (pendingResult_.hasValue())
                {
                    return;
                }

                HttpStatus status;
                std::string message;

                switch (error.code())
                {
                case lumalink::http::pipeline::PipelineErrorCode::InvalidVersion:
                case lumalink::http::pipeline::PipelineErrorCode::InvalidMethod:
                    status = HttpStatus::BadRequest();
                    message = "Bad Request: ";
                    break;
                case lumalink::http::pipeline::PipelineErrorCode::UriTooLong:
                case lumalink::http::pipeline::PipelineErrorCode::HeaderTooLarge:
                case lumalink::http::pipeline::PipelineErrorCode::BodyTooLarge:
                    status = HttpStatus::PayloadTooLarge();
                    message = "Payload Too Large: ";
                    break;
                case lumalink::http::pipeline::PipelineErrorCode::Timeout:
                    status = HttpStatus::RequestTimeout();
                    message = "Request Timeout: ";
                    break;
                case lumalink::http::pipeline::PipelineErrorCode::UnsupportedMediaType:
                    status = HttpStatus::UnsupportedMediaType();
                    message = "Unsupported Media Type: ";
                    break;
                default:
                    status = HttpStatus::BadRequest();
                    message = "Bad Request: ";
                    break;
                }

                std::unique_ptr<lumalink::http::response::IHttpResponse> response =
                    lumalink::http::response::StringResponse::create(
                        status, message + std::string(error.message()), {});
                setPendingResult(lumalink::http::pipeline::RequestHandlingResult::response(lumalink::http::response::CreateResponseStream(std::move(response))));
                markResponseStarted();
            }

            void onClientDisconnected() override
            {
            }

            bool hasPendingResult() const override
            {
                return pendingResult_.hasValue();
            }

            lumalink::http::pipeline::RequestHandlingResult takeResult() override
            {
                lumalink::http::pipeline::RequestHandlingResult result = std::move(pendingResult_);
                pendingResult_ = lumalink::http::pipeline::RequestHandlingResult();
                return result;
            }

        private:
            HttpContext context_;
            std::unique_ptr<lumalink::http::handlers::IHttpHandler> handler_;
            lumalink::http::pipeline::RequestHandlingResult pendingResult_;
            std::size_t bodyBytesReceived_ = 0;
            HttpContextPhaseFlags completedPhases_ = 0;

            lumalink::http::handlers::IHttpHandler *tryGetHandler()
            {
                if (!handler_)
                {
                    handler_ = HttpContextAccess::createHandler(context_);
                }

                return handler_.get();
            }

            void setPendingResult(lumalink::http::pipeline::RequestHandlingResult result)
            {
                pendingResult_ = std::move(result);
            }

            void markResponseStarted()
            {
                completedPhases_ |= HttpContextPhase::WritingResponseStarted;
            }

            void handleStep()
            {
                if (pendingResult_.hasValue())
                {
                    return;
                }

                lumalink::http::handlers::IHttpHandler *handler = tryGetHandler();
                if (handler != nullptr)
                {
                    lumalink::http::handlers::HandlerResult handlerResult = handler->handleStep(context_);
                    if (handlerResult.hasValue())
                    {
                        switch (handlerResult.kind)
                        {
                        case lumalink::http::handlers::HandlerResult::Kind::Response:
                            setPendingResult(lumalink::http::pipeline::RequestHandlingResult::response(lumalink::http::response::CreateResponseStream(std::move(handlerResult.response))));
                            markResponseStarted();
                            break;
                        case lumalink::http::handlers::HandlerResult::Kind::Upgrade:
                            setPendingResult(lumalink::http::pipeline::RequestHandlingResult::upgrade(std::move(handlerResult.upgradedSession)));
                            break;
                        case lumalink::http::handlers::HandlerResult::Kind::None:
                        default:
                            break;
                        }

                        return;
                    }
                }

                if ((completedPhases_ & HttpContextPhase::CompletedReadingMessage) != 0 && !pendingResult_.hasValue())
                {
                    setPendingResult(lumalink::http::pipeline::RequestHandlingResult::noResponse());
                }
            }
        };
    }

    lumalink::http::pipeline::PipelineHandlerPtr HttpContext::createPipelineHandler(lumalink::http::server::HttpServerBase &server, IHttpContextHandlerFactory &handlerFactory)
    {
        return lumalink::http::pipeline::PipelineHandlerPtr(
            new HttpContextPipelineAdapter(std::make_unique<DefaultHttpContextRunner>(server, handlerFactory)),
            [](lumalink::http::pipeline::IPipelineHandler *handler)
            {
                delete handler;
            });
    }
} // namespace lumalink::http::core



