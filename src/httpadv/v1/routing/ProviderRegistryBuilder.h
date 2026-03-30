#pragma once
#include "../core/Defines.h"
#include "../handlers/HttpHandler.h"
#include "../handlers/HandlerTypes.h"

#include "HandlerMatcher.h"
#include "../handlers/HandlerRestrictions.h"
#include "HandlerProviderRegistry.h"
#include "HandlerBuilder.h"
#include "../websocket/WebSocketCallbacks.h"
#include "../websocket/WebSocketUpgradeHandler.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace httpadv::v1::routing
{
    using httpadv::v1::handlers::ExtractArgsFromRequest;
    using httpadv::v1::handlers::GetRequest;
    using httpadv::v1::handlers::HttpHandler;
    using httpadv::v1::handlers::IHttpHandler;
    using httpadv::v1::handlers::RouteParameters;
    using httpadv::v1::websocket::WebSocketCallbacks;
    using httpadv::v1::websocket::WebSocketUpgradeHandler;

    // Forward declaration
    template <typename THandler> class HandlerBuilder;

    class ProviderRegistryBuilder
    {
    public:
        using AddPosition = int;
        struct AddAt
        {
            static const AddPosition Beginning = 0;
            static const AddPosition End = -1;
        };

    private:
        HandlerProviderRegistry &providerRegistry_;
        std::size_t websocketHandlerCount_ = 0;

    public:
        ProviderRegistryBuilder(HandlerProviderRegistry &factory)
            : providerRegistry_(factory)
        {
        }
        ~ProviderRegistryBuilder() = default;

        void add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, AddPosition position = AddAt::End)
        {
           providerRegistry_.add(predicate, handler, position);
        }
        void add(IHttpHandler::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position = AddAt::End)
        {
            providerRegistry_.add(predicate, invocation, position);
        }

        inline void on(HandlerMatcher &request, IHttpHandler::Factory handler)
        {
            providerRegistry_.add(
                [&request](httpadv::v1::core::HttpContext &context)
                {
                    return request.canHandle(context);
                },
                std::move(handler));
        }

        ProviderRegistryBuilder &websocket(std::string_view path, WebSocketCallbacks callbacks)
        {
            std::string registeredPath(path);
            providerRegistry_.add(
                [registeredPath](httpadv::v1::core::HttpContext &context)
                {
                    return WebSocketUpgradeHandler::isWebSocketUpgradeCandidate(context) && defaultCheckUriPattern(context.uriView().path(), registeredPath);
                },
                [callbacks = std::move(callbacks)](httpadv::v1::core::HttpContext &) mutable -> std::unique_ptr<IHttpHandler>
                {
                    return std::make_unique<HttpHandler>(
                        [callbacks = std::move(callbacks)](httpadv::v1::core::HttpContext &context) mutable -> IHttpHandler::HandlerResult
                        {
                            WebSocketUpgradeHandler upgradeHandler;
                            return upgradeHandler.handle(context, callbacks);
                        });
                },
                static_cast<AddPosition>(websocketHandlerCount_));
            ++websocketHandlerCount_;

            return *this;
        }

        // Template methods for handler registration
        template <typename THandler, typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, const typename THandler::Invocation &handler)
        {
            HandlerMatcher req = request;
            THandler::restrict(req);
            // Adapt HandlerMatcher's ArgsExtractor (takes 2 params) to ExtractArgsFromRequest (takes 1 param)
            ExtractArgsFromRequest adapterExtractor = [req](httpadv::v1::core::HttpContext &context) { return req.extractParameters(context); };
            auto addHandler = [this](IHttpHandler::Predicate predicate, IHttpHandler::Factory factory)
            {
                providerRegistry_.add(std::move(predicate), std::move(factory));
            };
            return HandlerBuilder<THandler>(std::move(addHandler), request, handler, adapterExtractor);
        }

        template <typename THandler, typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, const typename THandler::InvocationWithoutParams &handler)
        {
            HandlerMatcher req = request;
            THandler::restrict(req);
            auto addHandler = [this](IHttpHandler::Predicate predicate, IHttpHandler::Factory factory)
            {
                providerRegistry_.add(std::move(predicate), std::move(factory));
            };
            return HandlerBuilder<THandler>(std::move(addHandler), request, handler);
        }
        template <typename THandler, typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, const typename THandler::Invocation &handler)
        {
            HandlerMatcher request(path);
            THandler::restrict(request);
            // Adapt HandlerMatcher's ArgsExtractor (takes 2 params) to ExtractArgsFromRequest (takes 1 param)
            ExtractArgsFromRequest adapterExtractor = [request](httpadv::v1::core::HttpContext &context) { return request.extractParameters(context); };
            auto addHandler = [this](IHttpHandler::Predicate predicate, IHttpHandler::Factory factory)
            {
                providerRegistry_.add(std::move(predicate), std::move(factory));
            };
            return HandlerBuilder<THandler>(std::move(addHandler), request, handler, adapterExtractor);
        }

        template <typename THandler, typename = std::enable_if_t<httpadv::v1::handlers::HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, const typename THandler::InvocationWithoutParams &handler)
        {
            HandlerMatcher request(path);
            THandler::restrict(request);
            auto addHandler = [this](IHttpHandler::Predicate predicate, IHttpHandler::Factory factory)
            {
                providerRegistry_.add(std::move(predicate), std::move(factory));
            };
            return HandlerBuilder<THandler>(std::move(addHandler), request, handler);
        }

        void onNotFound(IHttpHandler::InvocationCallback invocation)
        {
            providerRegistry_.setDefaultHandlerFactory([invocation](httpadv::v1::core::HttpContext &context)
                                              { return std::make_unique<HttpHandler>(invocation, [](const httpadv::v1::core::HttpContext &)
                                                                                     { return true; }); });
        }
    };
} // namespace httpadv::v1::routing


