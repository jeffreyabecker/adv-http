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

namespace httpadv::v1::routing
{
    using httpadv::v1::handlers::HandlerResult;
    using httpadv::v1::handlers::HandlerProvider;
    using httpadv::v1::handlers::HttpHandler;
    using httpadv::v1::response::IHttpResponse;

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

        static std::unique_ptr<httpadv::v1::handlers::IHttpHandler> createDefaultHandler(httpadv::v1::core::HttpRequestContext &context);
        std::unique_ptr<httpadv::v1::handlers::IHttpHandler> wrapHandler(std::unique_ptr<httpadv::v1::handlers::IHttpHandler> innerHandler) const;
        std::vector<std::reference_wrapper<httpadv::v1::handlers::IHandlerProvider>> factories_;
        std::vector<std::unique_ptr<httpadv::v1::handlers::IHandlerProvider>> ownedFactoryItems_;
        httpadv::v1::handlers::IHttpHandler::Factory defaultFactory_ = nullptr;
        httpadv::v1::handlers::IHttpHandler::Predicate globalRequestFilter_ = nullptr;
        httpadv::v1::handlers::IHttpHandler::InterceptorCallback globalRequestInterceptor_ = nullptr;
        IHttpResponse::ResponseFilter globalResponseFilter_ = nullptr;

        class ResponseFilterApplicator : public httpadv::v1::handlers::IHttpHandler
        {
        private:
            IHttpResponse::ResponseFilter filter_;
            httpadv::v1::handlers::IHttpHandler::InterceptorCallback interceptor_;
            std::unique_ptr<httpadv::v1::handlers::IHttpHandler> innerHandler_;

        public:
            ResponseFilterApplicator(std::unique_ptr<httpadv::v1::handlers::IHttpHandler> innerHandler, IHttpResponse::ResponseFilter filter, httpadv::v1::handlers::IHttpHandler::InterceptorCallback interceptor = nullptr)
                : filter_(filter), interceptor_(interceptor), innerHandler_(std::move(innerHandler)) {}
            virtual httpadv::v1::handlers::IHttpHandler::HandlerResult handleStep(httpadv::v1::core::HttpRequestContext &context) override
            {
                httpadv::v1::handlers::IHttpHandler::HandlerResult response = interceptor_ ? interceptor_(context, httpadv::v1::handlers::IHttpHandler::InvocationNext(context, [this, &context]()
                                                                                   { return innerHandler_->handleStep(context); }))
                                                                    : innerHandler_->handleStep(context);
                if (response.isResponse() && filter_)
                {
                    response = HandlerResult::responseResult(filter_(std::move(response.response)));
                }
                return response;
            }
            virtual void handleBodyChunk(httpadv::v1::core::HttpRequestContext &context, const uint8_t *at, std::size_t length) override
            {
                innerHandler_->handleBodyChunk(context, at, length);
            }
        };

    public:
        HandlerProviderRegistry() {}
        std::unique_ptr<httpadv::v1::handlers::IHttpHandler> createContextHandler(httpadv::v1::core::HttpRequestContext &context);
        void setDefaultHandlerFactory(httpadv::v1::handlers::IHttpHandler::Factory creator);

        void add(httpadv::v1::handlers::IHandlerProvider &handlerFactory, AddPosition position = AddAt::End);
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

        void add(httpadv::v1::handlers::IHttpHandler::Predicate predicate, httpadv::v1::handlers::IHttpHandler::Factory handler, AddPosition position = AddAt::End);
        void add(httpadv::v1::handlers::IHttpHandler::Predicate predicate, httpadv::v1::handlers::IHttpHandler::InvocationCallback invocation, AddPosition position = AddAt::End);
        void with(httpadv::v1::handlers::IHttpHandler::InterceptorCallback interceptor);
        void filterRequest(httpadv::v1::handlers::IHttpHandler::Predicate predicate);
        void apply(IHttpResponse::ResponseFilter filter);
        // void setGlobalRequestFilter(IHttpHandler::Predicate predicate);
        // void getGlobalRequestFilter(IHttpHandler::Predicate &predicate);
        // void addResponseFilter(IHttpResponse::ResponseFilter filter);
    };
}



