#include "./HttpServerBase.h"

namespace HttpServerAdvanced {

HttpServerBase::HttpServerBase()
    : pipelineHandlerFactory_(nullptr),
      currentPipeline_(nullptr) {
}

HttpServerBase::~HttpServerBase() {
    end();
}

void HttpServerBase::handleClient() {
    if (pipelineHandlerFactory_) {
        if (!currentPipeline_) {
            std::unique_ptr<IClient> accepted = accept();
            if (accepted) {
                PipelineHandlerPtr handler = pipelineHandlerFactory_(*this);
                currentPipeline_ = std::make_unique<HttpPipeline>(
                    std::move(accepted),
                    *this,
                    timeouts_,
                    std::move(handler));
            }
        }
        if (currentPipeline_) {
            auto result = currentPipeline_->handleClient();
            if (isPipelineHandleClientResultFinal(result)) {
                currentPipeline_ = nullptr;
            }
        }
    }
    else {
        assert(false && "No Pipeline Handler Factory was setup");
    }
}

void HttpServerBase::begin() {
}

void HttpServerBase::end() {
    currentPipeline_ = nullptr;
}

HttpTimeouts &HttpServerBase::timeouts() {
    return timeouts_;
}

void HttpServerBase::setTimeouts(const HttpTimeouts &timeouts) {
    timeouts_ = timeouts;
}

std::map<String, std::any> &HttpServerBase::items() const {
    return items_;
}

bool HttpServerBase::hasService(const String &serviceName) const {
    return items_.find(serviceName) != items_.end();
}

HttpServerBase &HttpServerBase::use(std::function<void(HttpServerBase &)> setupFunc) {
    setupFunc(static_cast<HttpServerBase &>(*this));
    return static_cast<HttpServerBase &>(*this);
}

void HttpServerBase::addService(HttpServerBase &server, const String &serviceName, std::any serviceInstance) {
    items_[serviceName] = serviceInstance;
}

void HttpServerBase::setPipelineHandlerFactory(std::function<PipelineHandlerPtr(HttpServerBase &)> factory) {
    pipelineHandlerFactory_ = factory;
}

} // namespace HttpServerAdvanced
