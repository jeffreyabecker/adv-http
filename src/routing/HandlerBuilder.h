#pragma once
#include "../core/Defines.h"
#include "../handlers/HandlerRestrictions.h"

#include "HandlerMatcher.h"

#include <functional>
#include <string_view>

namespace HttpServerAdvanced
{
    template <typename THandler>
    class HandlerBuilder
    {
    private:
        std::function<void(IHttpHandler::Predicate, IHttpHandler::Factory)> addHandler_;
        IHttpHandler::Predicate predicate_ = nullptr;
        HandlerMatcher matcher_;
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
        static RouteParameters EmptyParameters(HttpRequest &context)
        {
            return {};
        }

    public:
        HandlerBuilder(std::function<void(IHttpHandler::Predicate, IHttpHandler::Factory)> addHandler, HandlerMatcher matcher,
                       typename THandler::InvocationWithoutParams invocationCallback)
            : addHandler_(std::move(addHandler)),
              matcher_(std::move(matcher)),
              predicate_(nullptr),
              invocationCallback_(THandler::curryWithoutParams(invocationCallback)),
              extractor_(EmptyParameters)
        {
        }

        HandlerBuilder(std::function<void(IHttpHandler::Predicate, IHttpHandler::Factory)> addHandler, HandlerMatcher matcher,
                       typename THandler::Invocation invocationCallback,
                       ExtractArgsFromRequest extractor)
            : addHandler_(std::move(addHandler)),
              matcher_(std::move(matcher)),
              predicate_(nullptr),
              invocationCallback_(invocationCallback),
              extractor_(extractor)
        {
        }

        ~HandlerBuilder()
        {
            if (addHandler_)
            {

                IHttpHandler::Predicate predicate = [matcher = std::move(matcher_), predicate = std::move(predicate_)](HttpRequest &context)
                {
                    if (predicate)
                    {
                        if (!predicate(context))
                        {
                            return false;
                        }
                    }
                    return matcher.canHandle(context);
                };

                addHandler_(predicate, getFactory());
                predicate_ = nullptr;
                addHandler_ = nullptr;
            }
        }

        HandlerBuilder(const HandlerBuilder &) = delete;
        HandlerBuilder &operator=(const HandlerBuilder &) = delete;

        HandlerBuilder(HandlerBuilder &&other) noexcept
            : addHandler_(std::move(other.addHandler_)),
              predicate_(std::move(other.predicate_)),
              matcher_(std::move(other.matcher_)),
              invocationCallback_(std::move(other.invocationCallback_)),
              extractor_(std::move(other.extractor_))
        {
            other.addHandler_ = nullptr;
        }

        HandlerBuilder &operator=(HandlerBuilder &&other) noexcept
        {
            if (this != &other)
            {
                addHandler_ = std::move(other.addHandler_);
                predicate_ = std::move(other.predicate_);
                matcher_ = std::move(other.matcher_);
                invocationCallback_ = std::move(other.invocationCallback_);
                extractor_ = std::move(other.extractor_);
                other.addHandler_ = nullptr;
            }
            return *this;
        }
        /**
         * @brief Wrap the handler invocation with an interceptor.
         * @param wrapper The interceptor callback that wraps the handler invocation.
         * @return Reference to this HandlerBuilder for method chaining.
         * @details The interceptor can perform actions before and after the handler invocation,
         * modify the request context, or even short-circuit the handler invocation. For example
         * the interceptor can be used to implement logging, authentication, or other cross-cutting concerns.
         * A concrete example is the BasicAuthentication implementation.
         */
        HandlerBuilder &with(IHttpHandler::InterceptorCallback wrapper)
        {
            invocationCallback_ = THandler::curryInterceptor(wrapper, invocationCallback_);
            return *this;
        }

        /**
         * @brief Adds a predicate to filter requests for this handler.
         * @param predicate The predicate to filter requests.
         * @return Reference to this HandlerBuilder for method chaining.
         * @details The predicate is combined with any existing predicates using logical AND.
         * This allows for more granular control over which requests are handled by this handler.
         * For example, you can filter requests based on headers, query parameters, or other request properties.
         */

        HandlerBuilder &filterRequest(IHttpHandler::Predicate predicate)
        {
            auto originalPredicate = predicate_;
            predicate_ = [originalPredicate, predicate](HttpRequest &context)
            {
                if (originalPredicate && !originalPredicate(context))
                {
                    return false;
                }
                return predicate(context);
            };
            return *this;
        }
        /**
         * @brief Applies a response filter to modify the response generated by the handler.
         * @param filter The response filter to apply.
         * @return Reference to this HandlerBuilder for method chaining.
         * @details The response filter can modify, replace, or wrap the response generated by the handler.
         * Multiple response filters can be applied, and they will be executed in the order they were added.
         * This allows for flexible response manipulation, such as adding headers, changing status codes,
         * or transforming the response body. A concrete example is adding CORS headers to responses.
         */
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

        HandlerBuilder &allowMethods(std::string_view methods)
        {
            matcher_.setAllowedMethods(methods);
            return *this;
        }

        HandlerBuilder &allowContentTypes(const std::initializer_list<std::string_view> &contentTypes)
        {
            matcher_.setAllowedContentTypes(contentTypes);
            return *this;
        }
    };

} // namespace HttpServerAdvanced


