#pragma once
#include <functional>
#include <string>
#include <vector>
#include "../core/Defines.h"
#include "../response/IHttpResponse.h"
#include "IHttpHandler.h"
namespace HttpServerAdvanced
{
    class HandlerProviderRegistry;
    class HandlerMatcher;
    
    // Forward declare for trait checking
    using RouteParameter = std::string;
    using RouteParameters = std::vector<RouteParameter>;
    using ExtractArgsFromRequest = std::function<RouteParameters(class HttpRequest &context)>;

    namespace HandlerRestrictions
    {
        template <typename T, typename = void>
        struct has_without_params_callback : std::false_type
        {
        };

        template <typename T>
        struct has_without_params_callback<T, std::void_t<typename T::InvocationWithoutParams>> : std::true_type
        {
        };

        template <typename T, typename = void>
        struct has_with_params_callback : std::false_type
        {
        };

        template <typename T>
        struct has_with_params_callback<T, std::void_t<typename T::Invocation>> : std::true_type
        {
        };

        template <typename T, typename = void>
        struct has_correct_constructor : std::false_type
        {
        };

        template <typename T, typename = void>
        struct has_curry_without_params : std::false_type
        {
        };

        template <typename T>
        struct has_curry_without_params<T, std::void_t<decltype(T::curryWithoutParams(std::declval<typename T::InvocationWithoutParams>()))>> : std::true_type
        {
        };

        template <typename T, typename = void>
        struct has_make_factory : std::false_type
        {
        };

        template <typename T>
        struct has_make_factory<T, std::void_t<decltype(T::makeFactory(std::declval<typename T::Invocation>(), std::declval<ExtractArgsFromRequest>()))>> : std::true_type
        {
        };

        template <typename T, typename = void>
        struct has_curry_interceptor : std::false_type
        {
        };

        template <typename T>
        struct has_curry_interceptor<T, std::void_t<decltype(T::curryInterceptor(std::declval<typename IHttpHandler::InterceptorCallback>(), std::declval<typename T::Invocation>()))>> : std::true_type
        {
        };

        // requires: static Invocation applyFilter(IHttpHandler::InterceptorCallback, Invocation)
        template <typename T, typename = void>
        struct has_apply_filter : std::false_type
        {
        };

        template <typename T>
        struct has_apply_filter<T, std::void_t<decltype(T::applyFilter(std::declval<typename IHttpHandler::InterceptorCallback>(), std::declval<typename T::Invocation>()))>> : std::true_type
        {
        };

        // requires: static Invocation applyResponseFilter(IHttpResponse::ResponseFilter, Invocation)
        template <typename T, typename = void>
        struct has_apply_response_filter : std::false_type
        {
        };

        template <typename T>
        struct has_apply_response_filter<T, std::void_t<decltype(T::applyResponseFilter(std::declval<IHttpResponse::ResponseFilter>(), std::declval<typename T::Invocation>()))>> : std::true_type
        {
        };

        template <typename T>
        struct is_valid_handler_type : std::conjunction<has_without_params_callback<T>, has_with_params_callback<T>,
                                                        has_curry_without_params<T>,
                                                        has_make_factory<T>,
                                                        has_apply_filter<T>,
                                                        has_apply_response_filter<T>>
        {
        };

        template <typename T, typename = void>
        struct has_restrict : std::false_type
        {
        };

        template <typename T>
        struct has_restrict<T, std::void_t<decltype(T::restrict(std::declval<HandlerMatcher&>()))>> : std::true_type
        {
        };
    }
} // namespace HttpServerAdvanced

