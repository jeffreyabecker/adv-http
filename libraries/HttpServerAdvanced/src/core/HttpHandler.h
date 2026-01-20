#pragma once
#include <memory>
#include <functional>

#include "../util/Util.h"

#include "./HttpMethod.h"
#include "./HttpContext.h"

namespace HttpServerAdvanced::Core
{
    class HttpContext;
    class IHttpResponse;
    class IHttpHandler
    {
    public:
        using HandlerResult = std::unique_ptr<IHttpResponse>;
        using InvocationCallback = std::function<IHttpHandler::HandlerResult(HttpContext &context)>;
        using InterceptorCallback = std::function<IHttpHandler::HandlerResult(HttpContext &context, InvocationCallback next)>;
        virtual ~IHttpHandler() = default;
        /**
         * @brief Handles the given HttpContext.
         * @param context The HttpContext to handle.
         * @details This method is called multiple times across the lifetime of the HttpContext.
         * depending on when the handler is invoked in the pipeline, it can be called *many* times per context.
         * The handler should inspect the context to determine what work needs to be done. handle is potentially
         * called:
         * - After the request line has been read (when URL and method are available)
         * - After headers have been read
         * - After body has been read
         */
        virtual HandlerResult handleStep(HttpContext &context) = 0;

        /**
         * @brief Appends body contents to the request.
         * @param context The HttpContext whose request body is being appended to.
         * @param at Pointer to the body data.
         * @param length Length of the body data.
         */
        virtual void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) = 0;
    };

    class HttpHandler : public IHttpHandler
    {
    protected:
        static const HttpContextPhaseFlags CallHandleAt = HttpContextPhase::CompletedReadingHeaders + HttpContextPhase::CompletedStartingLine;
        std::function<bool(const HttpContext &)> filter_;
        IHttpHandler::InvocationCallback invocation_;

    public:
        static bool defaultFilter(const HttpContext &context) { return context.completedPhases() == HttpContextPhase::CompletedReadingHeaders + HttpContextPhase::CompletedStartingLine; }

        HttpHandler(IHttpHandler::InvocationCallback invocation)
            : invocation_(invocation), filter_(defaultFilter) {}

        HttpHandler(IHttpHandler::InvocationCallback invocation, std::function<bool(const HttpContext &)> filter)
            : invocation_(invocation), filter_(filter) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response)
            : invocation_([response = std::move(response)](HttpContext &context) mutable -> IHttpHandler::HandlerResult
                          { return std::move(response); }),
              filter_(defaultFilter) {}

        HttpHandler(std::unique_ptr<IHttpResponse> response, std::function<bool(const HttpContext &)> filter)
            : invocation_([response = std::move(response)](HttpContext &context) mutable -> IHttpHandler::HandlerResult
                          { return std::move(response); }),
              filter_(filter) {}

        virtual ~HttpHandler() = default;

        virtual HandlerResult handleStep(HttpContext &context) override
        {
            if (filter_(context))
            {
                return invocation_(context);
            }
            return nullptr;
        }

        virtual void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) override
        {
            // Default implementation does nothing
        }
    };

    class BufferingHttpHandlerBase : public IHttpHandler
    {
    private:
        std::vector<uint8_t> bodyBuffer_;
        size_t contentLength_{0};

    public:
        virtual ~BufferingHttpHandlerBase() = default;

        virtual HandlerResult handleBody(HttpContext &context, std::vector<uint8_t> &&body) = 0;
        HandlerResult handleStep(HttpContext &context) override;
        void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) override;
    };

    class HttpHandlerFactory
    {
    public:
        static constexpr const char *ServiceName = "HttpHandlerFactory";
        using Predicate = std::function<bool(HttpContext &)>;
        using Factory = std::function<std::unique_ptr<IHttpHandler>(HttpContext &)>;

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
            HttpHandlerFactory::Factory factory_;
            HttpHandlerFactory::Predicate request_;

        public:
            ClosureFactoryItem(HttpHandlerFactory::Factory factory, HttpHandlerFactory::Predicate request)
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
        static std::unique_ptr<IHttpHandler> createDefaultHandler(HttpContext &context)
        {
            return std::make_unique<HttpHandler>(
                HttpResponse::create(HttpStatus::NotFound(), "404 Not Found", HttpHeaders::ContentType("text/plain")),
                [](const HttpContext &)
                { return true; });
        }
        std::vector<std::reference_wrapper<IHttpHandlerFactoryItem>> factories_;
        std::vector<std::unique_ptr<IHttpHandlerFactoryItem>> ownedFactoryItems_;
        Factory defaultFactory_ = nullptr;
        Predicate globalRequestFilter_ = nullptr;
        IHttpResponse::ResponseFilter globalResponseFilter_ = nullptr;

        class ResponseFilterApplicator : public IHttpHandler
        {
        private:
            IHttpResponse::ResponseFilter filter_;
            std::unique_ptr<IHttpHandler> innerHandler_;

        public:
            ResponseFilterApplicator(std::unique_ptr<IHttpHandler> innerHandler, IHttpResponse::ResponseFilter filter)
                : innerHandler_(std::move(innerHandler)), filter_(filter) {}
            virtual HandlerResult handleStep(HttpContext &context) override
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
        std::unique_ptr<IHttpHandler> HttpHandlerFactory::createContextHandler(HttpContext &context)
        {
            if (!globalRequestFilter_ || globalRequestFilter_(context))
            {
                for (auto &creator : factories_)
                {
                    HttpHandlerFactory::IHttpHandlerFactoryItem &factory = creator.get();
                    if (factory.canHandle(const_cast<HttpContext &>(context)))
                    {
                        return factory.create(const_cast<HttpContext &>(context));
                    }
                }
            }
            auto inner = defaultFactory_ ? defaultFactory_(const_cast<HttpContext &>(context)) : createDefaultHandler(const_cast<HttpContext &>(context));
            if (globalResponseFilter_)
            {
                return std::make_unique<ResponseFilterApplicator>(std::move(inner), globalResponseFilter_);
            }
            return inner;
        }
        void setDefaultHandlerFactory(Factory creator)
        {
            defaultFactory_ = creator;
        }

        void add(IHttpHandlerFactoryItem &handlerFactory, AddPosition position = AddAt::End)
        {
            if (position == AddAt::Beginning)
            {
                factories_.insert(factories_.begin(), handlerFactory);
                return;
            }
            else if (position != AddAt::End)
            {
                size_t pos = static_cast<size_t>(position);
                if (pos > factories_.size())
                {
                    pos = factories_.size();
                }
                factories_.insert(factories_.begin() + pos, handlerFactory);
                return;
            }
            else
                factories_.emplace_back(handlerFactory);
        }
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

        void add(HttpHandlerFactory::Predicate predicate, HttpHandlerFactory::Factory handler, AddPosition position = AddAt::End)
        {
            auto item = std::make_unique<ClosureFactoryItem>(handler, predicate);
            auto &ref = *item;
            ownedFactoryItems_.push_back(std::move(item));
            add(ref, position);
        }
        void add(HttpHandlerFactory::Predicate predicate, IHttpHandler::InvocationCallback invocation, AddPosition position = AddAt::End)
        {
            add(predicate, [invocation](HttpContext &context)
                { return std::make_unique<HttpHandler>(invocation, [](const HttpContext &)
                                                       { return true; }); }, position);
        }
        void setGlobalRequestFilter(Predicate predicate)
        {
            globalRequestFilter_ = predicate;
        }
        void getGlobalRequestFilter(Predicate &predicate)
        {
            predicate = globalRequestFilter_;
        }
        void addResponseFilter(IHttpResponse::ResponseFilter filter)
        {
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
    };
}