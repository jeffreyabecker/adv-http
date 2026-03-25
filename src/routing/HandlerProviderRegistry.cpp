#include "HandlerProviderRegistry.h"
#include "../core/HttpContext.h"
#include "../handlers/HttpHandler.h"
#include "../response/HttpResponse.h"
#include "../response/StringResponse.h"

namespace HttpServerAdvanced
{

    std::unique_ptr<IHttpHandler> HandlerProviderRegistry::createDefaultHandler(HttpContext &context)
    {
        return std::make_unique<HttpHandler>(
            StringResponse::create(HttpStatus::NotFound(), "404 Not Found",
                                 {HttpHeader(HttpHeaderNames::ContentType, "text/plain")}),
            [](const HttpContext &)
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

    std::unique_ptr<IHttpHandler> HandlerProviderRegistry::createContextHandler(HttpContext &context)
    {
        if (!globalRequestFilter_ || globalRequestFilter_(context))
        {
            for (auto &creator : factories_)
            {
                IHandlerProvider &factory = creator.get();
                if (factory.canHandle(const_cast<HttpContext &>(context)))
                {
                    return wrapHandler(factory.create(const_cast<HttpContext &>(context)));
                }
            }
        }
        auto inner = defaultFactory_
                         ? defaultFactory_(const_cast<HttpContext &>(context))
                         : createDefaultHandler(const_cast<HttpContext &>(context));
        return wrapHandler(std::move(inner));
    }

    void HandlerProviderRegistry::setDefaultHandlerFactory(IHttpHandler::Factory creator)
    {
        defaultFactory_ = creator;
    }

    void HandlerProviderRegistry::add(IHandlerProvider &handlerFactory, AddPosition position)
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
        add(predicate, [invocation](HttpContext &context)
            { return std::make_unique<HttpHandler>(invocation, [](const HttpContext &)
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
            globalRequestFilter_ = [previousFilter, predicate](HttpContext &context) -> bool
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
            globalRequestInterceptor_ = [previousInterceptor, interceptor](HttpContext &context, IHttpHandler::InvocationCallback next) -> IHttpHandler::HandlerResult
            {
                return interceptor(context, [previousInterceptor, next](HttpContext &ctx) -> IHttpHandler::HandlerResult
                                   { return previousInterceptor(ctx, next); });
            };
        }
        else
        {
            globalRequestInterceptor_ = interceptor;
        }
    }

} // namespace HttpServerAdvanced





