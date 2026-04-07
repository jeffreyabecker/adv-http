#pragma once
#include <vector>
#include <cstring>
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include "IHttpHandler.h"
#include "../core/HttpRequestContext.h"
#include "../response/HttpResponse.h"
#include "../response/StringResponse.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../core/Defines.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "../core/HttpContentTypes.h"
#include "../util/HttpUtility.h"
namespace lumalink::http::handlers
{
    using lumalink::http::core::HttpRequestContext;
    using lumalink::http::core::HttpContentTypes;
    using lumalink::http::core::HttpHeader;
    using lumalink::http::core::HttpHeaderNames;
    using lumalink::http::core::HttpStatus;
    using lumalink::http::core::MULTIPART_FORM_DATA_BUFFER_SIZE;
    using lumalink::http::response::IHttpResponse;
    using lumalink::http::response::StringResponse;
    using lumalink::http::routing::HandlerMatcher;
    using lumalink::http::util::WebUtility;

    enum class MultipartStatus
    {
        FirstChunk,      // First chunk of a multipart form field
        SubsequentChunk, // Middle chunks of a multipart form field
        FinalChunk,      // Last chunk of a multipart form field (boundary found)
        Completed        // All multipart data processed
    };

    class MultipartFormDataBuffer
    {
    private:
        std::vector<uint8_t> data_;
        std::string_view filename_;
        std::string_view contentType_;
        std::string_view partName_;
        MultipartStatus status_ = MultipartStatus::FirstChunk;

    public:
        MultipartFormDataBuffer(const uint8_t *data, size_t size, std::string_view filename, std::string_view contentType, std::string_view name, MultipartStatus status)
            : data_(), filename_(filename), contentType_(contentType), partName_(name), status_(status)
        {
            if (data != nullptr && size > 0)
            {
                data_.assign(data, data + size);
            }
        }
        MultipartFormDataBuffer(std::vector<uint8_t> data, std::string_view filename, std::string_view contentType, std::string_view name, MultipartStatus status)
            : data_(std::move(data)), filename_(filename), contentType_(contentType), partName_(name), status_(status) {}
        const uint8_t *data() const { return data_.data(); }
        size_t size() const { return data_.size(); }
        const uint8_t *begin() const { return data_.data(); }
        const uint8_t *end() const { return data_.data() + data_.size(); }
        const std::vector<uint8_t> &bytes() const { return data_; }
        std::string_view filename() const { return filename_; }
        std::string_view contentType() const { return contentType_; }
        std::string_view name() const { return partName_; }
        std::string_view filenameView() const { return filename_; }
        std::string_view contentTypeView() const { return contentType_; }
        std::string_view nameView() const { return partName_; }
        MultipartStatus status() const { return status_; }
    };

    class MultipartFormDataHandler : public IHttpHandler
    {

    private:
        std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &, MultipartFormDataBuffer)> handler_;
        ExtractArgsFromRequest extractor_;
        HandlerResult response_;
        RouteParameters params_;
        std::string boundary_;
        std::string filename_;
        std::string contentType_;
        std::string partName_;
        uint8_t buffer_[MULTIPART_FORM_DATA_BUFFER_SIZE];
        size_t bufferLength_ = 0;
        bool parsingHeaders_ = true;
        bool partStarted_ = false; // Track if we've emitted first chunk of current part
        MultipartStatus status_ = MultipartStatus::FirstChunk;
        std::vector<uint8_t> partData_;

        // Extract boundary from Content-Type header
        void extractBoundary(std::string_view contentType);

        // Parse part headers (Content-Disposition, Content-Type) from buffer
        void parsePartHeaders();

        // Find boundary in buffer
        const uint8_t *findBoundary(const uint8_t *start, size_t len);

    public:
        MultipartFormDataHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &, MultipartFormDataBuffer)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        virtual HandlerResult handleStep(lumalink::http::core::HttpRequestContext &context)
        {
            if (!response_ && context.completedPhases() >= lumalink::http::core::HttpContextPhase::CompletedReadingMessage)
            {
                handleBodyChunk(context, nullptr, 0);
            }
            if (response_)
            {
                return std::move(response_);
            }
            return nullptr;
        }
        virtual void handleBodyChunk(lumalink::http::core::HttpRequestContext &context, const uint8_t *at, std::size_t length)
        {
            if (response_)
            {
                return;
            }

            // Extract boundary on first chunk
            if (boundary_.empty())
            {
                params_ = extractor_(context);
                std::optional<HttpHeader> contentTypeHeader = context.headers().find(HttpHeaderNames::ContentType);
                if (contentTypeHeader.has_value())
                {
                    extractBoundary(contentTypeHeader->valueView());
                }
                if (boundary_.empty())
                {
                    response_ = StringResponse::create(HttpStatus::BadRequest(), "Missing or invalid boundary", {});
                    return;
                }
            }

            // Append data to buffer
            if (at && length > 0)
            {
                if (bufferLength_ + length > sizeof(buffer_))
                {
                    length = sizeof(buffer_) - bufferLength_;
                }
                memcpy(buffer_ + bufferLength_, at, length);
                bufferLength_ += length;
            }

            // Parse part headers if needed
            if (parsingHeaders_ && bufferLength_ > 4)
            {
                parsePartHeaders();
            }

            // Look for boundary
            if (!parsingHeaders_)
            {
                const uint8_t *boundaryPos = findBoundary(buffer_, bufferLength_);
                if (boundaryPos)
                {
                    // Extract part data before boundary (final chunk)
                    size_t dataLen = boundaryPos - buffer_;
                    if (dataLen > 0)
                    {
                        partData_.insert(partData_.end(), buffer_, buffer_ + dataLen);
                    }

                    // Invoke handler with accumulated part data marked as final
                    if (!partName_.empty())
                    {
                        MultipartStatus finalStatus = partStarted_ ? MultipartStatus::FinalChunk : MultipartStatus::FirstChunk;
                        response_ = handler_(context, params_,
                                             MultipartFormDataBuffer(std::move(partData_),
                                                                     filename_, contentType_, partName_, finalStatus));
                    }

                    // Reset for next part
                    partStarted_ = false;
                    partData_ = {};
                    filename_.clear();
                    contentType_.clear();
                    partName_.clear();
                    parsingHeaders_ = true;

                    // Move to next part
                    size_t boundaryLen = boundary_.size() + 4; // +4 for \r\n and initial \r\n
                    if (boundaryPos - buffer_ + boundaryLen < bufferLength_)
                    {
                        memmove(buffer_, boundaryPos + boundaryLen, bufferLength_ - (boundaryPos - buffer_ + boundaryLen));
                        bufferLength_ -= (boundaryPos - buffer_ + boundaryLen);
                    }
                    else
                    {
                        bufferLength_ = 0;
                    }
                }
                else if (bufferLength_ > boundary_.size() + 4)
                {
                    // Keep boundary-sized tail; flush excess as part data via handler
                    // This enables streaming of large files
                    size_t flushLen = bufferLength_ - (boundary_.size() + 4);

                    if (!partName_.empty() && flushLen > 0)
                    {
                        partData_.insert(partData_.end(), buffer_, buffer_ + flushLen);

                        // Invoke handler with accumulated data to stream it
                        MultipartStatus chunkStatus = partStarted_ ? MultipartStatus::SubsequentChunk : MultipartStatus::FirstChunk;
                        response_ = handler_(context, params_,
                                             MultipartFormDataBuffer(std::move(partData_),
                                                                     filename_, contentType_, partName_, chunkStatus));

                        if (!response_)
                        {
                            partStarted_ = true; // Mark that we've emitted at least one chunk
                            partData_ = {};
                        }
                    }

                    memmove(buffer_, buffer_ + flushLen, boundary_.size() + 4);
                    bufferLength_ = boundary_.size() + 4;
                }
            }

            // If this is end-of-stream (no more data), flush any pending part and send Completed event
            if ((!at || length == 0))
            {
                // If there's a pending part with data, send it as final chunk first
                if (!partName_.empty() && !partData_.empty() && !response_)
                {
                    MultipartStatus finalStatus = partStarted_ ? MultipartStatus::FinalChunk : MultipartStatus::FirstChunk;
                    response_ = handler_(context, params_,
                                         MultipartFormDataBuffer(std::move(partData_),
                                                                 filename_, contentType_, partName_, finalStatus));
                    if (response_)
                    {
                        return;
                    }
                }

                // Send Completed event with everything empty
                if (!response_)
                {
                    filename_.clear();
                    contentType_.clear();
                    partName_.clear();
                    partData_ = {};
                    bufferLength_ = 0;
                    parsingHeaders_ = true;
                    partStarted_ = false;
                    response_ = handler_(context, params_, MultipartFormDataBuffer({}, filename_, contentType_, partName_, MultipartStatus::Completed));
                }
                return;
            }
        }
    };
    class Multipart
    {
    public:
        using PostBodyData = WebUtility::QueryParameters;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, PostBodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, PostBodyData &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpRequestContext &context, RouteParameters &&, PostBodyData &&postData)
            {
                return handler(context, std::move(postData));
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
        {
            return [handler, extractor](lumalink::http::core::HttpRequestContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                // MultipartFormDataHandler expects handler of signature (lumalink::http::core::HttpRequestContext&, RouteParameters&, MultipartFormDataBuffer)
                // We'll create a simpler handler that just accepts the multipart buffer
                auto wrappedHandler = [handler](HttpRequestContext &ctx, RouteParameters &params, MultipartFormDataBuffer buffer)
                {
                    // Handler expects PostBodyData (KeyValuePairView), not MultipartFormDataBuffer
                    // For now, just invoke with empty PostBodyData - this API mismatch needs design review
                    WebUtility::QueryParameters postData;
                    return handler(ctx, std::move(params), std::move(postData));
                };
                return std::make_unique<MultipartFormDataHandler>(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &, MultipartFormDataBuffer)>(wrappedHandler),
                                                                  ExtractArgsFromRequest([params](lumalink::http::core::HttpRequestContext &c)
                                                                                         {
                                                                                             (void)c;
                                                                                             return params;
                                                                                         }));
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequestContext &context, RouteParameters &&params, PostBodyData &&postData)
            {
                return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, params = std::move(params), postData = std::move(postData)]() mutable
                                   { return handler(context, std::move(params), std::move(postData)); }));
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequestContext &context, RouteParameters &&params, PostBodyData &&postData)
            {
                return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, params = std::move(params), postData = std::move(postData)]() mutable
                                   { return handler(context, std::move(params), std::move(postData)); }));
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpRequestContext &context, RouteParameters &&params, PostBodyData &&postData)
            {
                auto response = handler(context, std::move(params), std::move(postData));
                if (!response.isResponse())
                {
                    return response;
                }

                response.response = filter(std::move(response.response));
                return response;
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
            baseUri.setAllowedContentTypes({HttpContentTypes::MultipartFormData});
        }
    };
} // namespace lumalink::http::handlers


