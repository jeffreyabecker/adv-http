#pragma once
#include "HttpServerBase.h"
#include "WebServerBuilder.h"

#include <type_traits>
#include <utility>

namespace httpadv::v1::server
{   
    using httpadv::v1::core::HttpContentTypes;
    using httpadv::v1::handlers::IHttpHandler;
    using httpadv::v1::response::IHttpResponse;
    using httpadv::v1::routing::HandlerBuilder;
    using httpadv::v1::routing::HandlerMatcher;
    using httpadv::v1::routing::HandlerProviderRegistry;
    using httpadv::v1::routing::ProviderRegistryBuilder;

    class WebServerConfig
    {
    private:
        HttpServerBase &server_;
        WebServerBuilder &builder_;

    public:
        WebServerConfig(HttpServerBase &server, WebServerBuilder &builder) : server_(server), builder_(builder) {}
        template <typename TComponent>
        WebServerBuilder &use(TComponent &&component)
        {
            static_assert(std::is_invocable_v<TComponent, WebServerBuilder &>, "component must be invocable with WebServerBuilder&");
            return builder_.use(std::forward<TComponent>(component));
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
        template <typename THandler = httpadv::v1::handlers::GetRequest, typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, const typename THandler::Invocation &handler)
        {
            return builder_.handlers().on<THandler>(request, handler);
        }

        template <typename THandler = httpadv::v1::handlers::GetRequest,
                  typename TCallable,
                  typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value &&
                                              !std::is_same_v<std::decay_t<TCallable>, typename THandler::Invocation> &&
                                              !std::is_same_v<std::decay_t<TCallable>, typename THandler::InvocationWithoutParams> &&
                                              !std::is_same_v<std::decay_t<TCallable>, typename THandler::LegacyInvocation> &&
                                              !std::is_same_v<std::decay_t<TCallable>, typename THandler::LegacyInvocationWithoutParams> &&
                                              (std::is_constructible_v<typename THandler::Invocation, TCallable> ||
                                               std::is_constructible_v<typename THandler::InvocationWithoutParams, TCallable> ||
                                               std::is_constructible_v<typename THandler::LegacyInvocation, TCallable> ||
                                               std::is_constructible_v<typename THandler::LegacyInvocationWithoutParams, TCallable>)>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, TCallable &&handler)
        {
            return builder_.handlers().on<THandler>(request, std::forward<TCallable>(handler));
        }

        template <typename THandler = httpadv::v1::handlers::GetRequest, typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, const typename THandler::InvocationWithoutParams &handler)
        {
            return builder_.handlers().on<THandler>(request, handler);
        }
        template <typename THandler = httpadv::v1::handlers::GetRequest, typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, const typename THandler::Invocation &handler)
        {
            return builder_.handlers().on<THandler>(path, handler);
        }

        template <typename THandler = httpadv::v1::handlers::GetRequest,
                  typename TCallable,
                  typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value &&
                                              !std::is_same_v<std::decay_t<TCallable>, typename THandler::Invocation> &&
                                              !std::is_same_v<std::decay_t<TCallable>, typename THandler::InvocationWithoutParams> &&
                                              !std::is_same_v<std::decay_t<TCallable>, typename THandler::LegacyInvocation> &&
                                              !std::is_same_v<std::decay_t<TCallable>, typename THandler::LegacyInvocationWithoutParams> &&
                                              (std::is_constructible_v<typename THandler::Invocation, TCallable> ||
                                               std::is_constructible_v<typename THandler::InvocationWithoutParams, TCallable> ||
                                               std::is_constructible_v<typename THandler::LegacyInvocation, TCallable> ||
                                               std::is_constructible_v<typename THandler::LegacyInvocationWithoutParams, TCallable>)>>
        HandlerBuilder<THandler> on(const char *path, TCallable &&handler)
        {
            return builder_.handlers().on<THandler>(path, std::forward<TCallable>(handler));
        }

        template <typename THandler = httpadv::v1::handlers::GetRequest, typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value>>
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
        template <typename TPredicate,
                  typename = std::enable_if_t<!std::is_same_v<std::decay_t<TPredicate>, IHttpHandler::Predicate> &&
                                              std::is_constructible_v<IHttpHandler::Predicate, TPredicate>>>
        void filterRequest(TPredicate &&predicate)
        {
            filterRequest(IHttpHandler::Predicate(std::forward<TPredicate>(predicate)));
        }
        void with(IHttpHandler::InterceptorCallback wrapper)
        {
            builder_.handlerProviders().with(wrapper);
        }
        inline HttpServerBase &server()
        {
            return server_;
        }
    };
}

