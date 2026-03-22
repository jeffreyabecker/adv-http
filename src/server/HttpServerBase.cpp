#include "HttpServerBase.h"

namespace HttpServerAdvanced {

HttpServerBase::HttpServerBase() : pipelineHandlerFactory_(nullptr), clock_(&Compat::DefaultClock()) {}

HttpServerBase::~HttpServerBase() {
    end();
}

void HttpServerBase::handleClient() {
    if (!pipelineHandlerFactory_) {
        assert(false && "No Pipeline Handler Factory was setup");
        return;
    }

    // Accept new connections up to the configured maximum
    while (pipelines_.size() < HttpServerAdvanced::MAX_CONCURRENT_CONNECTIONS) {
        std::unique_ptr<IClient> accepted = accept();
        if (!accepted) {
            break;
        }
        PipelineHandlerPtr handler = pipelineHandlerFactory_(*this);
        pipelines_.emplace_back(std::make_unique<HttpPipeline>(std::move(accepted), *this, timeouts_, std::move(handler), clock()));
    }

    // Service existing pipelines. Remove finished ones.
    for (size_t i = 0; i < pipelines_.size();) {
        auto result = pipelines_[i]->handleClient();
        if (isPipelineHandleClientResultFinal(result)) {
            // Erase finished pipeline
            pipelines_.erase(pipelines_.begin() + i);
        } else {
            ++i;
        }
    }
}

void HttpServerBase::begin() {
}

void HttpServerBase::end() {
    // Close and drop all pipelines
    pipelines_.clear();
}

HttpTimeouts &HttpServerBase::timeouts() {
    return timeouts_;
}

void HttpServerBase::setTimeouts(const HttpTimeouts &timeouts) {
    timeouts_ = timeouts;
}

void HttpServerBase::setClock(const Compat::Clock &clock) {
    clock_ = &clock;
}

const Compat::Clock &HttpServerBase::clock() const {
    return *clock_;
}

// std::map<String, std::any> &HttpServerBase::items() const {
//     return items_;
// }

// bool HttpServerBase::hasService(const String &serviceName) const {
//     return items_.find(serviceName) != items_.end();
// }

// HttpServerBase &HttpServerBase::use(std::function<void(HttpServerBase &)> setupFunc) {
//     setupFunc(static_cast<HttpServerBase &>(*this));
//     return static_cast<HttpServerBase &>(*this);
// }

// void HttpServerBase::addService(HttpServerBase &server, const String &serviceName, std::any serviceInstance) {
//     items_[serviceName] = serviceInstance;
// }

void HttpServerBase::setPipelineHandlerFactory(std::function<PipelineHandlerPtr(HttpServerBase &)> factory) {
    pipelineHandlerFactory_ = factory;
}

} // namespace HttpServerAdvanced

