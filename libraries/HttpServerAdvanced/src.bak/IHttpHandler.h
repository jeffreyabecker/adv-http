#pragma once
#include <memory>
#include <functional>

namespace HttpServerAdvanced
{
    class HttpRequest;
    class IHttpResponse;
    class IHttpHandler
    {
    public:
        using HandlerResult = std::unique_ptr<IHttpResponse>;
        using InvocationCallback = std::function<IHttpHandler::HandlerResult(HttpRequest &context)>;
        using InterceptorCallback = std::function<IHttpHandler::HandlerResult(HttpRequest &context, InvocationCallback next)>;
        using Predicate = std::function<bool(HttpRequest &)>;
        using Factory = std::function<std::unique_ptr<IHttpHandler>(HttpRequest &)>;        
        virtual ~IHttpHandler() = default;
        /**
         * @brief Handles the given HttpRequest.
         * @param context The HttpRequest to handle.
         * @details This method is called multiple times across the lifetime of the HttpRequest.
         * depending on when the handler is invoked in the pipeline, it can be called *many* times per context.
         * The handler should inspect the context to determine what work needs to be done. handle is potentially
         * called:
         * - After the request line has been read (when URL and method are available)
         * - After headers have been read
         * - After body has been read
         */
        virtual HandlerResult handleStep(HttpRequest &context) = 0;

        /**
         * @brief Appends body contents to the request.
         * @param context The HttpRequest whose request body is being appended to.
         * @param at Pointer to the body data.
         * @param length Length of the body data.
         */
        virtual void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length) = 0;
    };

}
