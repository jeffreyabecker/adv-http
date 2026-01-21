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
namespace HttpServerAdvanced
{
    // Forward declarations
    class HttpContext;
    

    
    class HttpHandlerFactory
    {
    public:
        static constexpr const char *ServiceName = "HttpHandlerFactory";

        class IHttpHandlerFactoryItem
        {
            friend class HttpHandlerFactory;

        public:
            virtual ~IHttpHandlerFactoryItem() = default;
            virtual bool canHandle(HttpContext &context) = 0;
            virtual std::unique_ptr<IHttpHandler> create(HttpContext &context) = 0;
        };
        using AddPosition = int;
        struct AddAt
        {
            static const AddPosition Beginning = 0;
            static const AddPosition End = -1;
        };
        class ClosureFactoryItem : public HttpHandlerFactory::IHttpHandlerFactoryItem
        {
        private:
            IHttpHandler::Factory factory_;
            IHttpHandler::Predicate request_;

        public:
            ClosureFactoryItem(IHttpHandler::Factory factory, IHttpHandler::Predicate request)
                : factory_(factory), request_(request) {}
            virtual bool canHandle(HttpContext &context) override
            {
                return request_(context);
            }
            virtual std::unique_ptr<IHttpHandler> create(HttpContext &context) override
            {
                return factory_(context);
            }
        };

    private:
        static std::unique_ptr<IHttpHandler> createDefaultHandler(HttpContext &context);
        std::vector<std::reference_wrapper<IHttpHandlerFactoryItem>> factories_;
        std::vector<std::unique_ptr<IHttpHandlerFactoryItem>> ownedFactoryItems_;
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
            virtual IHttpHandler::HandlerResult handleStep(HttpContext &context) override
            {
                auto response = innerHandler_->handleStep(context);
                if (response && filter_)
                {
                    response = filter_(std::move(response));
                }
                return response;
            }
            virtual void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) override
            {
                innerHandler_->handleBodyChunk(context, at, length);
            }
        };

    public:
        HttpHandlerFactory() {}
        std::unique_ptr<IHttpHandler> createContextHandler(HttpContext &context);
        void setDefaultHandlerFactory(IHttpHandler::Factory creator);

        void add(IHttpHandlerFactoryItem &handlerFactory, AddPosition position = AddAt::End);
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


