#pragma once
#include <memory>
#include <functional>
#include <cstddef>
#include <cstdint>

#include "HandlerResult.h"

namespace httpadv::v1::core
{
    class HttpContext;
}

namespace httpadv::v1::handlers
{
    class IHttpHandler
    {
    public:
        using HandlerResult = httpadv::v1::handlers::HandlerResult;
        using InvocationCallback = std::function<HandlerResult(httpadv::v1::core::HttpContext &context)>;
        using InterceptorCallback = std::function<HandlerResult(httpadv::v1::core::HttpContext &context, InvocationCallback next)>;
        using Predicate = std::function<bool(httpadv::v1::core::HttpContext &)>;
        using Factory = std::function<std::unique_ptr<IHttpHandler>(httpadv::v1::core::HttpContext &)>;        
        virtual ~IHttpHandler() = default;
        /**
         * @brief Handles the given httpadv::v1::core::HttpContext.
         * @param context The httpadv::v1::core::HttpContext to handle.
         * @details This method is called multiple times across the lifetime of the httpadv::v1::core::HttpContext.
         * depending on when the handler is invoked in the pipeline, it can be called *many* times per context.
         * The handler should inspect the context to determine what work needs to be done. handle is potentially
         * called:
         * - After the request line has been read (when URL and method are available)
         * - After headers have been read
         * - After body has been read
         */
        virtual HandlerResult handleStep(httpadv::v1::core::HttpContext &context) = 0;

        /**
         * @brief Appends body contents to the request.
         * @param context The httpadv::v1::core::HttpContext whose request body is being appended to.
         * @param at Pointer to the body data.
         * @param length Length of the body data.
         */
        virtual void handleBodyChunk(httpadv::v1::core::HttpContext &context, const uint8_t *at, std::size_t length) = 0;
    };

}
