#include "HttpContextPipelineAdapter.h"

namespace HttpServerAdvanced
{
    HttpContextPipelineAdapter::HttpContextPipelineAdapter(std::unique_ptr<HttpContextRunner> runner)
        : runner_(std::move(runner))
    {
    }

    int HttpContextPipelineAdapter::onMessageBegin(const char *method,
                                                   std::uint16_t versionMajor,
                                                   std::uint16_t versionMinor,
                                                   std::string_view url)
    {
        const int result = runner_->onMessageBegin(method, versionMajor, versionMinor, url);
        if (result == 0)
        {
            runner_->advance(HttpContextPhase::CompletedStartingLine);
        }
        return result;
    }

    void HttpContextPipelineAdapter::setAddresses(std::string_view remoteAddress,
                                                  std::uint16_t remotePort,
                                                  std::string_view localAddress,
                                                  std::uint16_t localPort)
    {
        runner_->setAddresses(remoteAddress, remotePort, localAddress, localPort);
    }

    int HttpContextPipelineAdapter::onHeader(std::string_view field, std::string_view value)
    {
        return runner_->onHeader(field, value);
    }

    int HttpContextPipelineAdapter::onHeadersComplete()
    {
        const int result = runner_->onHeadersComplete();
        if (result == 0)
        {
            runner_->advance(HttpContextPhase::CompletedReadingHeaders);
        }
        return result;
    }

    int HttpContextPipelineAdapter::onBody(const std::uint8_t *at, std::size_t length)
    {
        return runner_->onBody(at, length);
    }

    int HttpContextPipelineAdapter::onMessageComplete()
    {
        runner_->advance(HttpContextPhase::CompletedReadingMessage);
        return 0;
    }

    void HttpContextPipelineAdapter::onError(PipelineError error)
    {
        runner_->onError(error);
    }

    void HttpContextPipelineAdapter::onResponseStarted()
    {
        runner_->advance(HttpContextPhase::WritingResponseStarted);
    }

    void HttpContextPipelineAdapter::onResponseCompleted()
    {
        runner_->advance(HttpContextPhase::CompletedWritingResponse);
    }

    void HttpContextPipelineAdapter::onClientDisconnected()
    {
        runner_->onClientDisconnected();
    }

    bool HttpContextPipelineAdapter::hasPendingResult() const
    {
        return runner_->hasPendingResult();
    }

    RequestHandlingResult HttpContextPipelineAdapter::takeResult()
    {
        return runner_->takeResult();
    }

    HttpContextRunner &HttpContextPipelineAdapter::runner()
    {
        return *runner_;
    }

    const HttpContextRunner &HttpContextPipelineAdapter::runner() const
    {
        return *runner_;
    }
}