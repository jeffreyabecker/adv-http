#pragma once
#include <Arduino.h>
#include <functional>
#include "./Defines.h"
#include "./HttpHandlerFactory.h"
#include "./HttpContext.h"
#include "./HttpResponseFilters.h"
#include "./HandlersBuilder.h"
#include "./HttpContentTypes.h"
#include <any>
namespace HttpServerAdvanced
{
    class CoreServicesBuilder;

    using CoreServicesSetupFunc = std::function<void(CoreServicesBuilder &)>;

    class CoreServicesBuilder //: ServiceManagerService<HttpServerAdvanced::HttpServerBase, CoreServicesBuilder>
    {
    private:
        // static CoreServicesBuilder instance;

        HttpHandlerFactory handlerFactory_;

        CoreServicesSetupFunc setupFunc_;
        HandlersBuilder handlersBuilder_;
        HttpContentTypes contentTypes_;
        HttpServerAdvanced::HttpServerBase *server_ = nullptr;

        friend std::function<void(HttpServerAdvanced::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc);

    protected:
        void init(HttpServerAdvanced::HttpServerBase &server)
        {
            server.setPipelineHandlerFactory(HttpContext::createPipelineHandler);
            server.addService<CoreServicesBuilder *>(server, ServiceName, this);

            server.addService<HttpHandlerFactory *>(server, HttpHandlerFactory::ServiceName, &handlerFactory_);
            server.addService<HttpContentTypes *>(server, HttpContentTypes::ServiceName, &contentTypes_);

            if (setupFunc_)
            {
                setupFunc_(*this);
            }
        }
        static constexpr const char *ServiceName = "CoreServices";

    public:
        CoreServicesBuilder(CoreServicesSetupFunc setupFunc)
            : setupFunc_(setupFunc), handlersBuilder_(handlerFactory_)
        {
        }
        ~CoreServicesBuilder();

        CoreServicesBuilder &use(std::function<void(CoreServicesBuilder &)> component)
        {
            component(*this);
            return *this;
        }

        HttpHandlerFactory &handlerFactory()
        {
            return handlerFactory_;
        }

        HandlersBuilder &handlers()
        {
            return handlersBuilder_;
        }
        HttpContentTypes &contentTypes()
        {
            return contentTypes_;
        }
    };
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