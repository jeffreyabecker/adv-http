#pragma once
#include <memory>
#include <functional>
#include <vector>

#include "../handlers/IHttpHandler.h"
#include "../response/HttpResponse.h"
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../handlers/HttpHandler.h"
#include "../response/IHttpResponse.h"
#include "../handlers/IHandlerProvider.h"

namespace lumalink::http::routing
{
    using lumalink::http::handlers::HandlerResult;
    using lumalink::http::handlers::HandlerProvider;
    using lumalink::http::handlers::HttpHandler;
    using lumalink::http::response::IHttpResponse;

    class HandlerProviderRegistry
    {
    public:
        static constexpr const char *ServiceName = "HandlerProviderRegistry";

        using AddPosition = int;
        struct AddAt
        {
            static const AddPosition Beginning = 0;
            static const AddPosition End = -1;
        };

        static std::unique_ptr<lumalink::http::handlers::IHttpHandler> createDefaultHandler(lumalink::http::core::HttpRequestContext &context);
        std::unique_ptr<lumalink::http::handlers::IHttpHandler> wrapHandler(std::unique_ptr<lumalink::http::handlers::IHttpHandler> innerHandler) const;
        std::vector<std::reference_wrapper<lumalink::http::handlers::IHandlerProvider>> factories_;
        std::vector<std::unique_ptr<lumalink::http::handlers::IHandlerProvider>> ownedFactoryItems_;
        lumalink::http::handlers::IHttpHandler::Factory defaultFactory_ = nullptr;
        lumalink::http::handlers::IHttpHandler::Predicate globalRequestFilter_ = nullptr;
        lumalink::http::handlers::IHttpHandler::InterceptorCallback globalRequestInterceptor_ = nullptr;
        IHttpResponse::ResponseFilter globalResponseFilter_ = nullptr;

        class ResponseFilterApplicator : public lumalink::http::handlers::IHttpHandler
        {
        private:
            IHttpResponse::ResponseFilter filter_;
            lumalink::http::handlers::IHttpHandler::InterceptorCallback interceptor_;
            std::unique_ptr<lumalink::http::handlers::IHttpHandler> innerHandler_;

        public:
            ResponseFilterApplicator(std::unique_ptr<lumalink::http::handlers::IHttpHandler> innerHandler, IHttpResponse::ResponseFilter filter, lumalink::http::handlers::IHttpHandler::InterceptorCallback interceptor = nullptr)
                : filter_(filter), interceptor_(interceptor), innerHandler_(std::move(innerHandler)) {}
            virtual lumalink::http::handlers::IHttpHandler::HandlerResult handleStep(lumalink::http::core::HttpRequestContext &context) override
            {
                lumalink::http::handlers::IHttpHandler::HandlerResult response = interceptor_ ? interceptor_(context, lumalink::http::handlers::IHttpHandler::InvocationNext(context, [this, &context]()
                                                                                   { return innerHandler_->handleStep(context); }))
                                                                    : innerHandler_->handleStep(context);
                if (response.isResponse() && filter_)
                {
                    response = HandlerResult::responseResult(filter_(std::move(response.response)));
                }
                return response;
            }
            virtual void handleBodyChunk(lumalink::http::core::HttpRequestContext &context, const uint8_t *at, std::size_t length) override
            {
                innerHandler_->handleBodyChunk(context, at, length);
            }
        };

    public:
        HandlerProviderRegistry() {}
        std::unique_ptr<lumalink::http::handlers::IHttpHandler> createContextHandler(lumalink::http::core::HttpRequestContext &context);
        void setDefaultHandlerFactory(lumalink::http::handlers::IHttpHandler::Factory creator);

        void add(lumalink::http::handlers::IHandlerProvider &handlerFactory, AddPosition position = AddAt::End);
        template <typename THandler, typename... Args>
        void add(Args &&...args)
        {
            auto item = std::make_unique<THandler>(std::forward<Args>(args)...);
            auto &ref = *item;
            ownedFactoryItems_.push_back(std::move(item));
            add(ref, AddAt::End);
        }
        template <typename THandler, typename... Args>
        void addAt(AddPosition position, Args &&...args)
        {
            auto item = std::make_unique<THandler>(std::forward<Args>(args)...);
            auto &ref = *item;
            ownedFactoryItems_.push_back(std::move(item));
            add(ref, position);
        }

        void add(lumalink::http::handlers::IHttpHandler::Predicate predicate, lumalink::http::handlers::IHttpHandler::Factory handler, AddPosition position = AddAt::End);
        void add(lumalink::http::handlers::IHttpHandler::Predicate predicate, lumalink::http::handlers::IHttpHandler::InvocationCallback invocation, AddPosition position = AddAt::End);
        void with(lumalink::http::handlers::IHttpHandler::InterceptorCallback interceptor);
        void filterRequest(lumalink::http::handlers::IHttpHandler::Predicate predicate);
        void apply(IHttpResponse::ResponseFilter filter);
        // void setGlobalRequestFilter(IHttpHandler::Predicate predicate);
        // void getGlobalRequestFilter(IHttpHandler::Predicate &predicate);
        // void addResponseFilter(IHttpResponse::ResponseFilter filter);
    };
}



