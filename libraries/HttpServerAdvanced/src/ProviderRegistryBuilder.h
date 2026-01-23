#pragma once
#include <Arduino.h>
#include <utility>
#include "./Defines.h"
#include "./HttpHandler.h"
#include "./HandlerTypes.h"

#include "./HandlerMatcher.h"
#include "./HandlerRestrictions.h"
#include "./HandlerProviderRegistry.h"
#include "./HandlerBuilder.h"

namespace HttpServerAdvanced
{
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
        std::vector<std::unique_ptr<IHandlerProvider>> handlerItems_;

    public:
        ProviderRegistryBuilder(HandlerProviderRegistry &factory) : providerRegistry_(factory) {}
        ~ProviderRegistryBuilder() = default;

        void add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, AddPosition position = AddAt::End)
        {
           providerRegistry_.add(predicate, handler, position);
        }
        void add(IHttpHandler::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position = AddAt::End)
        {
            providerRegistry_.add(predicate, invocation, position);
        }

        void on(HandlerMatcher &request, IHttpHandler::Factory handler);

        // Template methods for handler registration
        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, const typename THandler::Invocation &handler)
        {
            HandlerMatcher req = request;
            THandler::restrict(req);
            // Adapt HandlerMatcher's ArgsExtractor (takes 2 params) to ExtractArgsFromRequest (takes 1 param)
            ExtractArgsFromRequest adapterExtractor = [req](HttpRequest &context) { return req.extractParameters(context); };
            auto addHandler = [this](IHttpHandler::Predicate predicate, IHttpHandler::Factory factory)
            {
                providerRegistry_.add(std::move(predicate), std::move(factory));
            };
            return HandlerBuilder<THandler>(std::move(addHandler), request, handler, adapterExtractor);
        }

        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
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
        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, const typename THandler::Invocation &handler)
        {
            HandlerMatcher request(path);
            THandler::restrict(request);
            // Adapt HandlerMatcher's ArgsExtractor (takes 2 params) to ExtractArgsFromRequest (takes 1 param)
            ExtractArgsFromRequest adapterExtractor = [request](HttpRequest &context) { return request.extractParameters(context); };
            auto addHandler = [this](IHttpHandler::Predicate predicate, IHttpHandler::Factory factory)
            {
                providerRegistry_.add(std::move(predicate), std::move(factory));
            };
            return HandlerBuilder<THandler>(std::move(addHandler), request, handler, adapterExtractor);
        }

        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
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
            providerRegistry_.setDefaultHandlerFactory([invocation](HttpRequest &context)
                                              { return std::make_unique<HttpHandler>(invocation, [](const HttpRequest &)
                                                                                     { return true; }); });
        }
    };
} // namespace HttpServerAdvanced
