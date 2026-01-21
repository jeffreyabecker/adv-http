#pragma once
#include <Arduino.h>
#include "./Defines.h"
#include "./HttpHandler.h"
#include "./HandlerTypes.h"

#include "./HandlerMatcher.h"
#include "./HandlerRestrictions.h"
#include "./HttpHandlerFactory.h"

namespace HttpServerAdvanced
{
    // Forward declaration
    template <typename THandler>
    class HandlerBuilder;

    class HandlersBuilder
    {
    public:
        using AddPosition = int;
        struct AddAt
        {
            static const AddPosition Beginning = 0;
            static const AddPosition End = -1;
        };

    private:
        HttpHandlerFactory &factory_;
        std::vector<std::unique_ptr<HttpHandlerFactory::IHttpHandlerFactoryItem>> handlerItems_;

    public:
        HandlersBuilder(HttpHandlerFactory &factory) : factory_(factory) {}
        ~HandlersBuilder() = default;

        void add(HttpHandlerFactory::Predicate predicate, HttpHandlerFactory::Factory handler, AddPosition position = AddAt::End)
        {
           factory_.add(predicate, handler, position);
        }
        void add(HttpHandlerFactory::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position = AddAt::End)
        {
            factory_.add(predicate, invocation, position);
        }

        void on(HandlerMatcher &request, HttpHandlerFactory::Factory handler);

        // Template methods for handler registration
        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(HandlerMatcher request, typename THandler::Invocation handler)
        {
            THandler::restrict(request);
            return HandlerBuilder<THandler>(this, request, handler);
        }

        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(HandlerMatcher request, typename THandler::InvocationWithoutParams handler)
        {
            THandler::restrict(request);
            return HandlerBuilder<THandler>(this, request, handler);
        }
        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, typename THandler::Invocation handler)
        {
            HandlerMatcher request(path);
            THandler::restrict(request);
            return HandlerBuilder<THandler>(this, request, handler);
        }

        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, typename THandler::InvocationWithoutParams handler)
        {
            HandlerMatcher request(path);
            THandler::restrict(request);
            return HandlerBuilder<THandler>(this, request, THandler::curryWithoutParams(handler));
        }

        void onNotFound(IHttpHandler::InvocationCallback invocation)
        {
            factory_.setDefaultHandlerFactory([invocation](HttpContext &context)
                                              { return std::make_unique<HttpHandler>(invocation, [](const HttpContext &)
                                                                                     { return true; }); });
        }
    };
} // namespace HttpServerAdvanced
