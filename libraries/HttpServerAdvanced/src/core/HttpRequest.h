#pragma once
#include "../util/Util.h"
#include "./HttpHeader.h"


namespace HttpServerAdvanced::Core
{
    class HttpContext;
    class HttpRequest
    {
    private:
        friend class HttpContext;
        friend class HttpParserHandler;

        HttpContext &context_;
        const char *method_;
        String version_;
        String url_;
        UriView uriView_;
        HttpHeadersCollection headers_;

        IPAddress remoteIP_;
        uint16_t remotePort_;
        IPAddress localIP_;
        uint16_t localPort_;

    public:
        HttpRequest(HttpContext &context) : context_(context), method_(nullptr), version_(), url_(), uriView_(), headers_(), remoteIP_(), remotePort_(0), localIP_(), localPort_(0) {}
        virtual ~HttpRequest() = default;
        const String &version() const { return version_; }
        const char *method() const
        {
            return method_;
        }
        const String &url() const
        {
            return url_;
        }
        const UriView &uri() const
        {
            return uriView_;
        }
        const HttpHeadersCollection &headers() const
        {
            return headers_;
        }

        /**
         * @brief Gets the remote IP address.
         *
         * @return The IP address of the remote endpoint.
         */
        IPAddress remoteIP();

        /**
         * @brief Gets the remote port number.
         *
         * @return The port number of the remote endpoint.
         */
        uint16_t remotePort();

        /**
         * @brief Gets the local IP address.
         *
         * @return The IP address of the local endpoint.
         */
        IPAddress localIP();

        /**
         * @brief Gets the local port number.
         *
         * @return The port number of the local endpoint.
         */
        uint16_t localPort();


    };
}