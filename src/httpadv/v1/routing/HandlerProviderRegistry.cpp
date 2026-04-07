#include "HandlerProviderRegistry.h"
#include "../handlers/HttpHandler.h"
#include "../response/HttpResponse.h"
#include "../response/StringResponse.h"

namespace lumalink::http::routing
{

    using lumalink::http::core::HttpHeader;
    using lumalink::http::core::HttpHeaderNames;
    using lumalink::http::core::HttpStatus;
    using lumalink::http::handlers::HandlerProvider;
    using lumalink::http::handlers::HttpHandler;
    using lumalink::http::handlers::IHttpHandler;
    using lumalink::http::response::IHttpResponse;
    using lumalink::http::response::StringResponse;

    std::unique_ptr<IHttpHandler> HandlerProviderRegistry::createDefaultHandler(lumalink::http::core::HttpRequestContext &context)
    {
        return std::make_unique<HttpHandler>(
            StringResponse::create(HttpStatus::NotFound(), "404 Not Found",
                                 {HttpHeader(HttpHeaderNames::ContentType, "text/plain")}),
            [](const lumalink::http::core::HttpRequestContext &)
            { return true; });
    }

    std::unique_ptr<IHttpHandler> HandlerProviderRegistry::wrapHandler(std::unique_ptr<IHttpHandler> innerHandler) const
    {
        if (!innerHandler)
        {
            return nullptr;
        }

        if (!globalRequestInterceptor_ && !globalResponseFilter_)
        {
            return innerHandler;
        }

        return std::make_unique<ResponseFilterApplicator>(std::move(innerHandler), globalResponseFilter_, globalRequestInterceptor_);
    }

    std::unique_ptr<IHttpHandler> HandlerProviderRegistry::createContextHandler(lumalink::http::core::HttpRequestContext &context)
    {
        if (!globalRequestFilter_ || globalRequestFilter_(context))
        {
            for (auto &creator : factories_)
            {
                lumalink::http::handlers::IHandlerProvider &factory = creator.get();
                if (factory.canHandle(context))
                {
                    return wrapHandler(factory.create(context));
                }
            }
        }
        auto inner = defaultFactory_
                         ? defaultFactory_(context)
                         : createDefaultHandler(context);
        return wrapHandler(std::move(inner));
    }

    void HandlerProviderRegistry::setDefaultHandlerFactory(IHttpHandler::Factory creator)
    {
        defaultFactory_ = creator;
    }

    void HandlerProviderRegistry::add(lumalink::http::handlers::IHandlerProvider &handlerFactory, AddPosition position)
    {
        if (position == AddAt::Beginning)
        {
            factories_.insert(factories_.begin(), std::ref(handlerFactory));
            return;
        }
        else if (position != AddAt::End)
        {
            size_t pos = static_cast<size_t>(position);
            if (pos > factories_.size())
            {
                pos = factories_.size();
            }
            factories_.insert(factories_.begin() + pos, std::ref(handlerFactory));
            return;
        }
        else
            factories_.emplace_back(std::ref(handlerFactory));
    }

    void HandlerProviderRegistry::add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, AddPosition position)
    {
        auto item = std::make_unique<HandlerProvider>(handler, predicate);
        auto &ref = *item;
        ownedFactoryItems_.push_back(std::move(item));
        add(ref, position);
    }

    void HandlerProviderRegistry::add(IHttpHandler::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position)
    {
        add(predicate, [invocation](lumalink::http::core::HttpRequestContext &context)
            { return std::make_unique<HttpHandler>(invocation, [](const lumalink::http::core::HttpRequestContext &)
                                                   { return true; }); }, position);
    }

    void HandlerProviderRegistry::filterRequest(IHttpHandler::Predicate predicate)
    {
        if (!predicate)
        {
            return;
        }

        if( globalRequestFilter_)
        {
            auto previousFilter = globalRequestFilter_;
            globalRequestFilter_ = [previousFilter, predicate](lumalink::http::core::HttpRequestContext &context) -> bool
            {
                return previousFilter(context) && predicate(context);
            };
        }
        else{
            globalRequestFilter_ = predicate;
        }
    }
    void HandlerProviderRegistry::apply(IHttpResponse::ResponseFilter filter)
    {
        if (!filter)
        {
            return;
        }

        if (globalResponseFilter_)
        {
            auto previousFilter = globalResponseFilter_;
            globalResponseFilter_ = [previousFilter, filter](std::unique_ptr<IHttpResponse> resp) -> std::unique_ptr<IHttpResponse>
            {
                return filter(previousFilter(std::move(resp)));
            };
        }
        else
        {
            globalResponseFilter_ = filter;
        }
    }
    void HandlerProviderRegistry::with(IHttpHandler::InterceptorCallback interceptor)
    {
        if (!interceptor)
        {
            return;
        }

        if (globalRequestInterceptor_)
        {
            auto previousInterceptor = globalRequestInterceptor_;
            globalRequestInterceptor_ = [previousInterceptor, interceptor](lumalink::http::core::HttpRequestContext &context, IHttpHandler::InvocationNext next) -> IHttpHandler::HandlerResult
            {
                return interceptor(context, IHttpHandler::InvocationNext(context, [previousInterceptor, &context, next]() mutable -> IHttpHandler::HandlerResult
                                   { return previousInterceptor(context, next); }));
            };
        }
        else
        {
            globalRequestInterceptor_ = interceptor;
        }
    }

} // namespace lumalink::http::routing





