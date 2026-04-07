#pragma once
#include <memory>
#include <functional>
#include <cstddef>
#include <cstdint>

#include "HandlerResult.h"

namespace lumalink::http::core
{
    class HttpRequestContext;
}

namespace lumalink::http::handlers
{
    class IHttpHandler
    {
    public:
        using HandlerResult = lumalink::http::handlers::HandlerResult;
        class InvocationNext
        {
        public:
            using Callback = std::function<HandlerResult()>;

        private:
            lumalink::http::core::HttpRequestContext *context_ = nullptr;
            Callback callback_;

        public:
            InvocationNext() = default;

            InvocationNext(lumalink::http::core::HttpRequestContext &context, Callback callback)
                : context_(&context), callback_(std::move(callback)) {}

            HandlerResult operator()() const
            {
                return callback_ ? callback_() : HandlerResult();
            }

            explicit operator bool() const
            {
                return static_cast<bool>(callback_);
            }

            lumalink::http::core::HttpRequestContext &context() const
            {
                return *context_;
            }
        };

        using InvocationCallback = std::function<HandlerResult(lumalink::http::core::HttpRequestContext &context)>;
        using InterceptorCallback = std::function<HandlerResult(lumalink::http::core::HttpRequestContext &context, InvocationNext next)>;
        using Predicate = std::function<bool(lumalink::http::core::HttpRequestContext &)>;
        using Factory = std::function<std::unique_ptr<IHttpHandler>(lumalink::http::core::HttpRequestContext &)>;
        virtual ~IHttpHandler() = default;
        /**
         * @brief Handles the given lumalink::http::core::HttpContext.
         * @param context The lumalink::http::core::HttpContext to handle.
         * @details This method is called multiple times across the lifetime of the lumalink::http::core::HttpContext.
         * depending on when the handler is invoked in the pipeline, it can be called *many* times per context.
         * The handler should inspect the context to determine what work needs to be done. handle is potentially
         * called:
         * - After the request line has been read (when URL and method are available)
         * - After headers have been read
         * - After body has been read
         */
        virtual HandlerResult handleStep(lumalink::http::core::HttpRequestContext &context) = 0;

        /**
         * @brief Appends body contents to the request.
         * @param context The lumalink::http::core::HttpContext whose request body is being appended to.
         * @param at Pointer to the body data.
         * @param length Length of the body data.
         */
        virtual void handleBodyChunk(lumalink::http::core::HttpRequestContext &context, const uint8_t *at, std::size_t length) = 0;
    };

}
