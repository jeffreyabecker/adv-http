#include "./CoreServices.h"
#include "./HttpServerBase.h"
#include "./HttpContext.h"

namespace HttpServerAdvanced {

void CoreServicesBuilder::init(HttpServerAdvanced::HttpServerBase &server) {
    server.setPipelineHandlerFactory(HttpContext::createPipelineHandler);
    server.addService<CoreServicesBuilder *>(server, ServiceName, this);

    server.addService<HttpHandlerFactory *>(server, HttpHandlerFactory::ServiceName, &handlerFactory_);
    server.addService<HttpContext::HandlerFactoryFunction>(server, HttpContext::HandlerFactoryServiceName,
                                [this](HttpContext &context) {
                                    return handlerFactory_.createContextHandler(context);
                                });
    server.addService<HttpContentTypes *>(server, HttpContentTypes::ServiceName, &contentTypes_);

    if (setupFunc_) {
        setupFunc_(*this);
    }
}

CoreServicesBuilder::CoreServicesBuilder(CoreServicesSetupFunc setupFunc)
    : setupFunc_(setupFunc), handlersBuilder_(handlerFactory_) {}

CoreServicesBuilder::~CoreServicesBuilder() = default;

CoreServicesBuilder &CoreServicesBuilder::use(std::function<void(CoreServicesBuilder &)> component) {
    component(*this);
    return *this;
}

HttpHandlerFactory &CoreServicesBuilder::handlerFactory() { return handlerFactory_; }
HandlersBuilder &CoreServicesBuilder::handlers() { return handlersBuilder_; }
HttpContentTypes &CoreServicesBuilder::contentTypes() { return contentTypes_; }

std::function<void(HttpServerAdvanced::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc) {
    static CoreServicesBuilder *instance = nullptr;
    if (instance == nullptr) {
        instance = new CoreServicesBuilder(setupFunc);
    }
    return [](HttpServerAdvanced::HttpServerBase &server) {
        instance->server_ = &server;
        if (!server.hasService(CoreServicesBuilder::ServiceName)) {
            instance->init(server);
        }
    };
}

} // namespace HttpServerAdvanced
