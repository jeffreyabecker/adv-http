#include "HandlerProviderRegistry.h"
#include "../core/HttpContext.h"
#include "../handlers/HttpHandler.h"
#include "../response/HttpResponse.h"
#include "../response/StringResponse.h"

namespace httpadv::v1::routing
{

    using httpadv::v1::core::HttpHeader;
    using httpadv::v1::core::HttpHeaderNames;
    using httpadv::v1::core::HttpStatus;
    using httpadv::v1::handlers::HandlerProvider;
    using httpadv::v1::handlers::HttpHandler;
    using httpadv::v1::handlers::IHttpHandler;
    using httpadv::v1::response::IHttpResponse;
    using httpadv::v1::response::StringResponse;

    std::unique_ptr<IHttpHandler> HandlerProviderRegistry::createDefaultHandler(httpadv::v1::core::HttpContext &context)
    {
        return std::make_unique<HttpHandler>(
            StringResponse::create(HttpStatus::NotFound(), "404 Not Found",
                                 {HttpHeader(HttpHeaderNames::ContentType, "text/plain")}),
            [](const httpadv::v1::core::HttpContext &)
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

    std::unique_ptr<IHttpHandler> HandlerProviderRegistry::createContextHandler(httpadv::v1::core::HttpContext &context)
    {
        if (!globalRequestFilter_ || globalRequestFilter_(context))
        {
            for (auto &creator : factories_)
            {
                httpadv::v1::handlers::IHandlerProvider &factory = creator.get();
                if (factory.canHandle(const_cast<httpadv::v1::core::HttpContext &>(context)))
                {
                    return wrapHandler(factory.create(const_cast<httpadv::v1::core::HttpContext &>(context)));
                }
            }
        }
        auto inner = defaultFactory_
                         ? defaultFactory_(const_cast<httpadv::v1::core::HttpContext &>(context))
                         : createDefaultHandler(const_cast<httpadv::v1::core::HttpContext &>(context));
        return wrapHandler(std::move(inner));
    }

    void HandlerProviderRegistry::setDefaultHandlerFactory(IHttpHandler::Factory creator)
    {
        defaultFactory_ = creator;
    }

    void HandlerProviderRegistry::add(httpadv::v1::handlers::IHandlerProvider &handlerFactory, AddPosition position)
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

    void HandlerProviderRegistry::add(std::function<bool(httpadv::v1::core::HttpContext &)> predicate, IHttpHandler::Factory handler, AddPosition position)
    {
        add(IHttpHandler::Predicate([predicate = std::move(predicate)](httpadv::v1::core::HttpRequestContext &context)
        {
            return predicate(static_cast<httpadv::v1::core::HttpContext &>(context));
        }), std::move(handler), position);
    }

    void HandlerProviderRegistry::add(IHttpHandler::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position)
    {
        add(predicate, [invocation](httpadv::v1::core::HttpContext &context)
            { return std::make_unique<HttpHandler>(invocation, [](const httpadv::v1::core::HttpContext &)
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
            globalRequestFilter_ = [previousFilter, predicate](httpadv::v1::core::HttpRequestContext &context) -> bool
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
            globalRequestInterceptor_ = [previousInterceptor, interceptor](httpadv::v1::core::HttpRequestContext &context, IHttpHandler::InvocationNext next) -> IHttpHandler::HandlerResult
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

} // namespace httpadv::v1::routing





