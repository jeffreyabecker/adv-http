#include "./BufferingHttpHandlerBase.h"
#include "./HttpRequest.h"
#include "./HttpHeader.h"
#include "./HttpRequestPhase.h"
#include <optional>

namespace HttpServerAdvanced {

IHttpHandler::HandlerResult BufferingHttpHandlerBase::handleStep(HttpRequest &context) {
    if (context.completedPhases() < HttpRequestPhase::CompletedReadingMessage) {
        return nullptr;
    }

    if (bodyBuffer_.empty()) {
        return nullptr;
    }

    return handleBody(context, std::move(bodyBuffer_));
}

void BufferingHttpHandlerBase::handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length) {
    if (contentLength_ == 0) {
        std::optional<HttpHeader> contentLengthHeader = context.request().headers().find("Content-Length");
        if (contentLengthHeader.has_value()) {
            contentLength_ = contentLengthHeader->value().toInt();
            bodyBuffer_.reserve(contentLength_);
        }
    }

    bodyBuffer_.insert(bodyBuffer_.end(), at, at + length);
    if (contentLength_ > 0 && bodyBuffer_.size() > contentLength_) {
        bodyBuffer_.resize(contentLength_);
    }

    if (contentLength_ > 0 && bodyBuffer_.size() == contentLength_) {
        handleBody(context, std::move(bodyBuffer_));
    }
}

} // namespace HttpServerAdvanced
