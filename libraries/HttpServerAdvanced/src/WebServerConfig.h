#pragma once
#include "./StandardHttpServer.h"
#include "./CoreServices.h"

namespace HttpServerAdvanced
{   
    class WebServerConfig
    {
    private:
        HttpServerBase &server_;
        CoreServicesBuilder &builder_;

    public:
        WebServerConfig(HttpServerBase &server, CoreServicesBuilder &builder) : server_(server), builder_(builder) {}
        CoreServicesBuilder &use(std::function<void(CoreServicesBuilder &)> component)
        {
            return builder_.use(component);
        }
        HandlerProviderRegistry &handlerProviders()
        {
            return builder_.handlerProviders();
        }
        ProviderRegistryBuilder &handlers()
        {
            return builder_.handlers();
        }
        HttpContentTypes &contentTypes()
        {
            return builder_.contentTypes();
        }
        void add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, ProviderRegistryBuilder::AddPosition position = ProviderRegistryBuilder::AddAt::End)
        {
            builder_.handlers().add(predicate, handler, position);
        }
        void add(IHttpHandler::Predicate predicate, IHttpHandler::InvocationCallback invocation, ProviderRegistryBuilder::AddPosition position = ProviderRegistryBuilder::AddAt::End)
        {
            builder_.handlers().add(predicate, invocation, position);
        }

        void on(HandlerMatcher &request, IHttpHandler::Factory handler)
        {
            builder_.handlers().on(request, handler);
        }

        // Template methods for handler registration
        template <typename THandler = HttpServerAdvanced::GetRequest, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, const typename THandler::Invocation &handler)
        {
            return builder_.handlers().on<THandler>(request, handler);
        }

        template <typename THandler = HttpServerAdvanced::GetRequest, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, const typename THandler::InvocationWithoutParams &handler)
        {
            return builder_.handlers().on<THandler>(request, handler);
        }
        template <typename THandler = HttpServerAdvanced::GetRequest, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, const typename THandler::Invocation &handler)
        {
            return builder_.handlers().on<THandler>(path, handler);
        }

        template <typename THandler = HttpServerAdvanced::GetRequest, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, const typename THandler::InvocationWithoutParams &handler)
        {
            return builder_.handlers().on<THandler>(path, handler);
        }

        void onNotFound(IHttpHandler::InvocationCallback invocation)
        {
            builder_.handlers().onNotFound(invocation);
        }
        void apply(IHttpResponse::ResponseFilter filter)
        {
            builder_.handlerProviders().apply(filter);
        }
        void filterRequest(IHttpHandler::Predicate predicate)
        {
            builder_.handlerProviders().filterRequest(predicate);
        }
        void interceptRequest(IHttpHandler::InterceptorCallback wrapper)
        {
            builder_.handlerProviders().interceptRequest(wrapper);
        }
    };
}