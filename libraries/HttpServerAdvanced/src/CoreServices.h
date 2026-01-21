#pragma once
#include <Arduino.h>
#include <functional>
#include "./Defines.h"
#include "./HandlerProviderRegistry.h"
#include "./HttpRequest.h"
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

        HandlerProviderRegistry handlerFactory_;

        CoreServicesSetupFunc setupFunc_;
        HandlersBuilder handlersBuilder_;
        HttpContentTypes contentTypes_;
        HttpServerAdvanced::HttpServerBase *server_ = nullptr;

        friend std::function<void(HttpServerAdvanced::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc);

    protected:
        void init(HttpServerAdvanced::HttpServerBase &server);
        static constexpr const char *ServiceName = "CoreServices";

    public:
        CoreServicesBuilder(CoreServicesSetupFunc setupFunc);
        ~CoreServicesBuilder();

        CoreServicesBuilder &use(std::function<void(CoreServicesBuilder &)> component);

        HandlerProviderRegistry &handlerFactory();

        HandlersBuilder &handlers();
        HttpContentTypes &contentTypes();
    };
    std::function<void(HttpServerAdvanced::HttpServerBase &)> CoreServices(std::function<void(CoreServicesBuilder &)> setupFunc);

} // namespace HttpServerAdvanced