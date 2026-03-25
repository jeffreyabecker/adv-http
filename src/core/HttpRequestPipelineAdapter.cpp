#include "HttpRequestPipelineAdapter.h"

namespace HttpServerAdvanced
{
    HttpRequestPipelineAdapter::HttpRequestPipelineAdapter(std::unique_ptr<HttpRequestRunner> runner)
        : runner_(std::move(runner))
    {
    }

    int HttpRequestPipelineAdapter::onMessageBegin(const char *method,
                                                   std::uint16_t versionMajor,
                                                   std::uint16_t versionMinor,
                                                   std::string_view url)
    {
        const int result = runner_->onMessageBegin(method, versionMajor, versionMinor, url);
        if (result == 0)
        {
            runner_->advance(HttpRequestPhase::CompletedStartingLine);
        }
        return result;
    }

    void HttpRequestPipelineAdapter::setAddresses(std::string_view remoteAddress,
                                                  std::uint16_t remotePort,
                                                  std::string_view localAddress,
                                                  std::uint16_t localPort)
    {
        runner_->setAddresses(remoteAddress, remotePort, localAddress, localPort);
    }

    int HttpRequestPipelineAdapter::onHeader(std::string_view field, std::string_view value)
    {
        return runner_->onHeader(field, value);
    }

    int HttpRequestPipelineAdapter::onHeadersComplete()
    {
        const int result = runner_->onHeadersComplete();
        if (result == 0)
        {
            runner_->advance(HttpRequestPhase::CompletedReadingHeaders);
        }
        return result;
    }

    int HttpRequestPipelineAdapter::onBody(const std::uint8_t *at, std::size_t length)
    {
        return runner_->onBody(at, length);
    }

    int HttpRequestPipelineAdapter::onMessageComplete()
    {
        runner_->advance(HttpRequestPhase::CompletedReadingMessage);
        return 0;
    }

    void HttpRequestPipelineAdapter::onError(PipelineError error)
    {
        runner_->onError(error);
    }

    void HttpRequestPipelineAdapter::onResponseStarted()
    {
        runner_->advance(HttpRequestPhase::WritingResponseStarted);
    }

    void HttpRequestPipelineAdapter::onResponseCompleted()
    {
        runner_->advance(HttpRequestPhase::CompletedWritingResponse);
    }

    void HttpRequestPipelineAdapter::onClientDisconnected()
    {
        runner_->onClientDisconnected();
    }

    bool HttpRequestPipelineAdapter::hasPendingResult() const
    {
        return runner_->hasPendingResult();
    }

    RequestHandlingResult HttpRequestPipelineAdapter::takeResult()
    {
        return runner_->takeResult();
    }

    HttpRequestRunner &HttpRequestPipelineAdapter::runner()
    {
        return *runner_;
    }

    const HttpRequestRunner &HttpRequestPipelineAdapter::runner() const
    {
        return *runner_;
    }
}