#include "./CoreServices.h"
#include "../pipeline/Pipeline.h"
#include "./HttpContentTypes.h"
namespace HttpServerAdvanced::Core
{

    void CoreServicesBuilder::init(HttpServerAdvanced::Pipeline::HttpServerBase &server)
    {
        server.setPipelineHandlerFactory(&pipelineHandlerFactory_);
        server.addService<CoreServicesBuilder *>(server, ServiceName, this);

        server.addService<HttpHandlerFactory *>(server, HttpHandlerFactory::ServiceName, &handlerFactory_);
        server.addService<HttpContentTypes *>(server, HttpContentTypes::ServiceName, &contentTypes_);

        if (setupFunc_)
        {
            setupFunc_(*this);
        }
    }

    CoreServicesBuilder::CoreServicesBuilder(CoreServicesSetupFunc setupFunc) : pipelineHandlerFactory_(), handlerFactory_(), contentTypes_(), setupFunc_(setupFunc), handlersBuilder_(handlerFactory_) {}

    CoreServicesBuilder::~CoreServicesBuilder() {}

    std::function<void(HttpServerAdvanced::Pipeline::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc)
    {
        static CoreServicesBuilder *instance = nullptr;
        if (instance == nullptr)
        {
            instance = new CoreServicesBuilder(setupFunc);
            
        }
        return [](HttpServerAdvanced::Pipeline::HttpServerBase &server)
        {
            instance->server_ = &server;
            if (!server.hasService(CoreServicesBuilder::ServiceName))
            {
                instance->init(server);
            }
        };
    }
    CoreServicesBuilder &CoreServicesBuilder::use(std::function<void(CoreServicesBuilder &)> component)
    {
        component(*this);
        return *this;
    }

} // namespace HttpServerAdvanced::Core