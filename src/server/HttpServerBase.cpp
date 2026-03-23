#include "HttpServerBase.h"

#include <cassert>

namespace HttpServerAdvanced {

HttpServerBase::HttpServerBase(std::unique_ptr<IServer> server)
    : pipelineHandlerFactory_(nullptr), server_(std::move(server)), clock_(&Compat::DefaultClock()) {
    assert(server_ && "HttpServerBase requires a valid transport server");
}

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
        if (!server_) {
            return;
        }

        std::unique_ptr<IClient> accepted = server_->accept();
        if (!accepted) {
            break;
        }
        pipelines_.emplace_back(std::make_unique<HttpPipeline>(
            std::move(accepted),
            *this,
            timeouts_,
            [this]() { return pipelineHandlerFactory_(*this); },
            clock()));
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
    if (!server_) {
        return;
    }

    server_->begin();
}

void HttpServerBase::end() {
    // Close and drop all pipelines
    pipelines_.clear();

    if (!server_) {
        return;
    }

    server_->end();
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

std::string_view HttpServerBase::localAddress() const {
    if (!server_) {
        return {};
    }

    return server_->localAddress();
}

uint16_t HttpServerBase::localPort() const {
    if (!server_) {
        return 0;
    }

    return server_->port();
}

// std::map<std::string, std::any> &HttpServerBase::items() const {
//     return items_;
// }

// bool HttpServerBase::hasService(const std::string &serviceName) const {
//     return items_.find(serviceName) != items_.end();
// }

// HttpServerBase &HttpServerBase::use(std::function<void(HttpServerBase &)> setupFunc) {
//     setupFunc(static_cast<HttpServerBase &>(*this));
//     return static_cast<HttpServerBase &>(*this);
// }

// void HttpServerBase::addService(HttpServerBase &server, const std::string &serviceName, std::any serviceInstance) {
//     items_[serviceName] = serviceInstance;
// }

void HttpServerBase::setPipelineHandlerFactory(std::function<PipelineHandlerPtr(HttpServerBase &)> factory) {
    pipelineHandlerFactory_ = factory;
}

} // namespace HttpServerAdvanced

