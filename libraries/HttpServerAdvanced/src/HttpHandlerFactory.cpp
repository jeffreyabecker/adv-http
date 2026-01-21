#include "./HttpHandlerFactory.h"
#include "./HttpContext.h"
#include "./HttpHandler.h"
#include "./HttpResponse.h"

namespace HttpServerAdvanced {

std::unique_ptr<IHttpHandler> HttpHandlerFactory::createDefaultHandler(HttpContext &context) {
    return std::make_unique<HttpHandler>(
        HttpResponse::create(HttpStatus::NotFound(), "404 Not Found", 
            HttpHeadersCollection{HttpHeader(HttpHeader::ContentType, "text/plain")}),
        [](const HttpContext &) { return true; });
}

std::unique_ptr<IHttpHandler> HttpHandlerFactory::createContextHandler(HttpContext &context) {
    if (!globalRequestFilter_ || globalRequestFilter_(context)) {
        for (auto &creator : factories_) {
            HttpHandlerFactory::IHttpHandlerFactoryItem &factory = creator.get();
            if (factory.canHandle(const_cast<HttpContext &>(context))) {
                return factory.create(const_cast<HttpContext &>(context));
            }
        }
    }
    auto inner = defaultFactory_ 
        ? defaultFactory_(const_cast<HttpContext &>(context)) 
        : createDefaultHandler(const_cast<HttpContext &>(context));
    if (globalResponseFilter_) {
        return std::make_unique<ResponseFilterApplicator>(std::move(inner), globalResponseFilter_);
    }
    return inner;
}

void HttpHandlerFactory::setDefaultHandlerFactory(IHttpHandler::Factory creator) {
    defaultFactory_ = creator;
}

void HttpHandlerFactory::add(IHttpHandlerFactoryItem &handlerFactory, AddPosition position) {
    if (position == AddAt::Beginning) {
        factories_.insert(factories_.begin(), handlerFactory);
        return;
    }
    else if (position != AddAt::End) {
        size_t pos = static_cast<size_t>(position);
        if (pos > factories_.size()) {
            pos = factories_.size();
        }
        factories_.insert(factories_.begin() + pos, handlerFactory);
        return;
    }
    else
        factories_.emplace_back(handlerFactory);
}

void HttpHandlerFactory::add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, AddPosition position) {
    auto item = std::make_unique<ClosureFactoryItem>(handler, predicate);
    auto &ref = *item;
    ownedFactoryItems_.push_back(std::move(item));
    add(ref, position);
}

void HttpHandlerFactory::add(IHttpHandler::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position) {
    add(predicate, [invocation](HttpContext &context) {
        return std::make_unique<HttpHandler>(invocation, [](const HttpContext &) { return true; });
    }, position);
}

void HttpHandlerFactory::setGlobalRequestFilter(IHttpHandler::Predicate predicate) {
    globalRequestFilter_ = predicate;
}

void HttpHandlerFactory::getGlobalRequestFilter(IHttpHandler::Predicate &predicate) {
    predicate = globalRequestFilter_;
}

void HttpHandlerFactory::addResponseFilter(IHttpResponse::ResponseFilter filter) {
    if (globalResponseFilter_) {
        auto previousFilter = globalResponseFilter_;
        globalResponseFilter_ = [previousFilter, filter](std::unique_ptr<IHttpResponse> resp) -> std::unique_ptr<IHttpResponse> {
            return filter(previousFilter(std::move(resp)));
        };
    }
    else {
        globalResponseFilter_ = filter;
    }
}

} // namespace HttpServerAdvanced
