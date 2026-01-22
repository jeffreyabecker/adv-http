#pragma once
#include <Arduino.h>
#include "./Defines.h"

namespace HttpServerAdvanced
{
    // Forward declaration
    class ProviderRegistryBuilder;

    template <typename THandler>
    class HandlerBuilder
    {
    private:
        ProviderRegistryBuilder *builder_ = nullptr;
        IHttpHandler::Predicate predicate_;
        typename THandler::Invocation invocationCallback_;
        ExtractArgsFromRequest extractor_;
        IHttpResponse::ResponseFilter responseFilter_ = nullptr;

        IHttpHandler::Factory getFactory()
        {
            auto invocation = invocationCallback_;
            if (responseFilter_)
            {
                invocation = THandler::applyResponseFilter(responseFilter_, invocation);
            }
            return THandler::makeFactory(invocation, extractor_);
        }
        static std::vector<String> EmptyParameters(HttpRequest &context)
        {
            return {};
        }

    public:
        HandlerBuilder(ProviderRegistryBuilder *builder, IHttpHandler::Predicate predicate,
                       typename THandler::InvocationWithoutParams invocationCallback)
            : builder_(builder),
              predicate_(predicate),
              invocationCallback_(THandler::curryWithoutParams(invocationCallback)),
              extractor_(EmptyParameters)
        {
        }

        HandlerBuilder(ProviderRegistryBuilder *builder, IHttpHandler::Predicate predicate,
                       typename THandler::Invocation invocationCallback,
                       ExtractArgsFromRequest extractor)
            : builder_(builder),
              predicate_(predicate),
              invocationCallback_(invocationCallback),
              extractor_(extractor)
        {
        }

        ~HandlerBuilder()
        {
            if (builder_)
            {
                builder_->add(predicate_, getFactory());
            }
        }

        HandlerBuilder(const HandlerBuilder &) = delete;
        HandlerBuilder &operator=(const HandlerBuilder &) = delete;

        HandlerBuilder(HandlerBuilder &&other) noexcept
            : builder_(other.builder_),
              predicate_(std::move(other.predicate_)),
              invocationCallback_(std::move(other.invocationCallback_)),
              extractor_(std::move(other.extractor_))
        {
            other.builder_ = nullptr;
        }

        HandlerBuilder &operator=(HandlerBuilder &&other) noexcept
        {
            if (this != &other)
            {
                builder_ = other.builder_;
                predicate_ = std::move(other.predicate_);
                invocationCallback_ = std::move(other.invocationCallback_);
                extractor_ = std::move(other.extractor_);
                other.builder_ = nullptr;
            }
            return *this;
        }

        HandlerBuilder &interceptRequest(IHttpHandler::InterceptorCallback wrapper)
        {
            invocationCallback_ = THandler::curryInterceptor(wrapper, invocationCallback_);
            return *this;
        }

        HandlerBuilder &filterRequest(IHttpHandler::Predicate predicate)
        {
            auto originalPredicate = predicate_;
            predicate_ = [originalPredicate, predicate](HttpRequest &context)
            {
                return predicate(context) && originalPredicate(context);
            };
            return *this;
        }

        HandlerBuilder &apply(IHttpResponse::ResponseFilter filter)
        {
            if (!filter)
            {
                return *this;
            }
            if (!responseFilter_)
            {
                responseFilter_ = filter;
                return *this;
            }
            auto originalFilter = responseFilter_;
            responseFilter_ = [originalFilter, filter](std::unique_ptr<IHttpResponse> resp)
            {
                return filter(originalFilter(std::move(resp)));
            };
            return *this;
        }
    };


} // namespace HttpServerAdvanced