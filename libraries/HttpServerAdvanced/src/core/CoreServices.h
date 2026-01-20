#pragma once
#include <Arduino.h>
#include "../pipeline/Pipeline.h"
#include "./HttpContext.h"
#include "./HttpResponseFilters.h"  
#include "./handlers/HandlerBuilder.h"
#include "./HttpContentTypes.h"
namespace HttpServerAdvanced::Core
{
    class CoreServicesBuilder;

    using CoreServicesSetupFunc = std::function<void(CoreServicesBuilder &)>;

    class CoreServicesBuilder : ServiceManagerService<HttpServerAdvanced::Pipeline::HttpServerBase, CoreServicesBuilder>
    {
    private:
        // static CoreServicesBuilder instance;
        ContextHttpPipelineHandlerFactory pipelineHandlerFactory_;
        HttpHandlerFactory handlerFactory_;
        HttpResponseFilters responseFilters_;
        CoreServicesSetupFunc setupFunc_;
        HandlersBuilder handlersBuilder_;
        HttpContentTypes contentTypes_;
        HttpServerAdvanced::Pipeline::HttpServerBase *server_ = nullptr;

        friend std::function<void(HttpServerAdvanced::Pipeline::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc);

    protected:
        void init(HttpServerAdvanced::Pipeline::HttpServerBase &server) ;
        static constexpr const char *ServiceName = "CoreServices";

    public:
        CoreServicesBuilder(CoreServicesSetupFunc setupFunc);
        ~CoreServicesBuilder();

        CoreServicesBuilder &use(std::function<void(CoreServicesBuilder &)> component);
        HttpResponseFilters &responseFilters()
        {
            return responseFilters_;
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
    std::function<void(HttpServerAdvanced::Pipeline::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc);

    

} // namespace HttpServerAdvanced::Core