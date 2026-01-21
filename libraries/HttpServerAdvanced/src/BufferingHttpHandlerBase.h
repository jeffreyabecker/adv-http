#pragma once
#include <vector>
#include "./IHttpHandler.h"

namespace HttpServerAdvanced
{
    // Forward declaration
    class HttpContext;

    class BufferingHttpHandlerBase : public IHttpHandler
    {
    private:
        std::vector<uint8_t> bodyBuffer_;
        size_t contentLength_{0};

    public:
        virtual ~BufferingHttpHandlerBase() = default;

        virtual HandlerResult handleBody(HttpContext &context, std::vector<uint8_t> &&body) = 0;
        HandlerResult handleStep(HttpContext &context) override;
        void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) override;
    };

}
