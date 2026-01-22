#include "./CoreServices.h"
#include "./HttpServerBase.h"
#include "./HttpRequest.h"
#include "./IHttpRequestHandlerFactory.h"
namespace HttpServerAdvanced
{
        // Factory that can create an IPipelineHandler owning a HttpRequest instance.
        // Uses a custom deleter so it's safe even when IPipelineHandler doesn't have a virtual dtor.


    void CoreServicesBuilder::init(HttpServerAdvanced::HttpServerBase &server)
    {
        server.setPipelineHandlerFactory(HttpRequest::createPipelineHandler);
        server.addService<CoreServicesBuilder *>(server, ServiceName, this);
        server.addService<HandlerProviderRegistry *>(server, HandlerProviderRegistry::ServiceName, &providerRegistry_);
        server.addService<IHttpRequestHandlerFactory *>(server, IHttpRequestHandlerFactory::ServiceName, &handlerFactory_);
        server.addService<HttpContentTypes *>(server, HttpContentTypes::ServiceName, &contentTypes_);

        if (setupFunc_)
        {
            setupFunc_(*this);
        }
    }

    CoreServicesBuilder::CoreServicesBuilder(CoreServicesSetupFunc setupFunc)
        : setupFunc_(setupFunc), handlersBuilder_(providerRegistry_), handlerFactory_(providerRegistry_) {}

    CoreServicesBuilder::~CoreServicesBuilder() = default;

    CoreServicesBuilder &CoreServicesBuilder::use(std::function<void(CoreServicesBuilder &)> component)
    {
        if (component)
        {
            component(*this);
        }
        return *this;
    }

    HandlerProviderRegistry &CoreServicesBuilder::handlerProviders() { return providerRegistry_; }
    ProviderRegistryBuilder &CoreServicesBuilder::handlers() { return handlersBuilder_; }
    HttpContentTypes &CoreServicesBuilder::contentTypes() { return contentTypes_; }

    std::function<void(HttpServerAdvanced::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc)
    {
        static CoreServicesBuilder *instance = nullptr;
        if (instance == nullptr)
        {
            instance = new CoreServicesBuilder(setupFunc);
        }
        return [](HttpServerAdvanced::HttpServerBase &server)
        {
            instance->server_ = &server;
            if (!server.hasService(CoreServicesBuilder::ServiceName))
            {
                instance->init(server);
            }
        };
    }

} // namespace HttpServerAdvanced
