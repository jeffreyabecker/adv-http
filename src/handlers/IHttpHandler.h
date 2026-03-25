#pragma once
#include <memory>
#include <functional>
#include <cstddef>
#include <cstdint>

#include "HandlerResult.h"

namespace HttpServerAdvanced
{
    class HttpContext;
    class IHttpHandler
    {
    public:
        using HandlerResult = HttpServerAdvanced::HandlerResult;
        using InvocationCallback = std::function<HandlerResult(HttpContext &context)>;
        using InterceptorCallback = std::function<HandlerResult(HttpContext &context, InvocationCallback next)>;
        using Predicate = std::function<bool(HttpContext &)>;
        using Factory = std::function<std::unique_ptr<IHttpHandler>(HttpContext &)>;        
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

}
