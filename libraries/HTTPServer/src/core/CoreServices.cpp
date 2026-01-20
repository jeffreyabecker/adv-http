#include "./CoreServices.h"
#include "../pipeline/Pipeline.h"
#include "./HttpContentTypes.h"
namespace HTTPServer::Core
{

    void CoreServicesBuilder::init(HTTPServer::Pipeline::HttpServerBase &server)
    {
        add<CoreServicesBuilder *>(server, ServiceName, this);
        server.setPipelineHandlerFactory(&pipelineHandlerFactory_);
        add<HttpHandlerFactory *>(server, HttpHandlerFactory::ServiceName, &handlerFactory_);
        add<HttpContentTypes *>(server, HttpContentTypes::ServiceName, &contentTypes_);

        if (setupFunc_)
        {
            setupFunc_(*this);
        }
    }

    CoreServicesBuilder::CoreServicesBuilder(CoreServicesSetupFunc setupFunc) : pipelineHandlerFactory_(), handlerFactory_(), contentTypes_(), setupFunc_(setupFunc), handlersBuilder_(handlerFactory_) {}

    CoreServicesBuilder::~CoreServicesBuilder() {}

    std::function<void(HTTPServer::Pipeline::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc)
    {
        static CoreServicesBuilder *instance = nullptr;
        if (instance == nullptr)
        {
            instance = new CoreServicesBuilder(setupFunc);
            
        }
        return [](HTTPServer::Pipeline::HttpServerBase &server)
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

} // namespace HTTPServer::Core