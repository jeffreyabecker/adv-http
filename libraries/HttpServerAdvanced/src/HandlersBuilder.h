#pragma once
#include <Arduino.h>
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
        HandlerProviderRegistry &factory_;
        std::vector<std::unique_ptr<IHandlerProvider>> handlerItems_;

    public:
        HandlersBuilder(HandlerProviderRegistry &factory) : factory_(factory) {}
        ~HandlersBuilder() = default;

        void add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, AddPosition position = AddAt::End)
        {
           factory_.add(predicate, handler, position);
        }
        void add(IHttpHandler::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position = AddAt::End)
        {
            factory_.add(predicate, invocation, position);
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
            return HandlerBuilder<THandler>(this, IHttpHandler::Predicate([req](HttpRequest &context) { return req(context); }), handler, adapterExtractor);
        }

        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const HandlerMatcher &request, const typename THandler::InvocationWithoutParams &handler)
        {
            HandlerMatcher req = request;
            THandler::restrict(req);
            return HandlerBuilder<THandler>(this, IHttpHandler::Predicate([req](HttpRequest &context) { return req(context); }), handler);
        }
        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, const typename THandler::Invocation &handler)
        {
            HandlerMatcher request(path);
            THandler::restrict(request);
            // Adapt HandlerMatcher's ArgsExtractor (takes 2 params) to ExtractArgsFromRequest (takes 1 param)
            ExtractArgsFromRequest adapterExtractor = [request](HttpRequest &context) { return request.extractParameters(context); };
            return HandlerBuilder<THandler>(this, IHttpHandler::Predicate([request](HttpRequest &context) { return request(context); }), handler, adapterExtractor);
        }

        template <typename THandler, typename = std::enable_if_t<HandlerRestrictions::is_valid_handler_type<THandler>::value>>
        HandlerBuilder<THandler> on(const char *path, const typename THandler::InvocationWithoutParams &handler)
        {
            HandlerMatcher request(path);
            THandler::restrict(request);
            return HandlerBuilder<THandler>(this, IHttpHandler::Predicate([request](HttpRequest &context) { return request(context); }), handler);
        }

        void onNotFound(IHttpHandler::InvocationCallback invocation)
        {
            factory_.setDefaultHandlerFactory([invocation](HttpRequest &context)
                                              { return std::make_unique<HttpHandler>(invocation, [](const HttpRequest &)
                                                                                     { return true; }); });
        }
    };
} // namespace HttpServerAdvanced
