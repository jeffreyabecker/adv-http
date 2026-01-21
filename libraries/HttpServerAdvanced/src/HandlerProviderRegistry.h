#pragma once
#include <memory>
#include <functional>
#include <vector>

#include "./IHttpHandler.h"
#include "./HttpResponse.h"
#include "./HttpStatus.h"
#include "./HttpHeader.h"
#include "./HttpHandler.h"
#include "./IHttpResponse.h"
#include "./IHandlerProvider.h"
namespace HttpServerAdvanced
{
    // Forward declarations
    class HttpRequest;

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

    private:
        static std::unique_ptr<IHttpHandler> createDefaultHandler(HttpRequest &context);
        std::vector<std::reference_wrapper<IHandlerProvider>> factories_;
        std::vector<std::unique_ptr<IHandlerProvider>> ownedFactoryItems_;
        IHttpHandler::Factory defaultFactory_ = nullptr;
        IHttpHandler::Predicate globalRequestFilter_ = nullptr;
        IHttpResponse::ResponseFilter globalResponseFilter_ = nullptr;

        class ResponseFilterApplicator : public IHttpHandler
        {
        private:
            IHttpResponse::ResponseFilter filter_;
            std::unique_ptr<IHttpHandler> innerHandler_;

        public:
            ResponseFilterApplicator(std::unique_ptr<IHttpHandler> innerHandler, IHttpResponse::ResponseFilter filter)
                : innerHandler_(std::move(innerHandler)), filter_(filter) {}
            virtual IHttpHandler::HandlerResult handleStep(HttpRequest &context) override
            {
                auto response = innerHandler_->handleStep(context);
                if (response && filter_)
                {
                    response = filter_(std::move(response));
                }
                return response;
            }
            virtual void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length) override
            {
                innerHandler_->handleBodyChunk(context, at, length);
            }
        };

    public:
        HandlerProviderRegistry() {}
        std::unique_ptr<IHttpHandler> createContextHandler(HttpRequest &context);
        void setDefaultHandlerFactory(IHttpHandler::Factory creator);

        void add(IHandlerProvider &handlerFactory, AddPosition position = AddAt::End);
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

        void add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, AddPosition position = AddAt::End);
        void add(IHttpHandler::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position = AddAt::End);
        void setGlobalRequestFilter(IHttpHandler::Predicate predicate);
        void getGlobalRequestFilter(IHttpHandler::Predicate &predicate);
        void addResponseFilter(IHttpResponse::ResponseFilter filter);
    };
}
