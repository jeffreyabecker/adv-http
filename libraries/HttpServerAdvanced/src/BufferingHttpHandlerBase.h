#pragma once
#include <vector>
#include "./IHttpHandler.h"

namespace HttpServerAdvanced
{
    // Forward declaration
    class HttpRequest;

    class BufferingHttpHandlerBase : public IHttpHandler
    {
    private:
        std::vector<uint8_t> bodyBuffer_;
        size_t contentLength_{0};

    public:
        virtual ~BufferingHttpHandlerBase() = default;

        virtual HandlerResult handleBody(HttpRequest &context, std::vector<uint8_t> &&body) = 0;
        HandlerResult handleStep(HttpRequest &context) override;
        void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length) override;
    };

}
