#pragma once

#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../server/HttpServerBase.h"
#include "../core/HttpContextPhase.h"
#include "../pipeline/IPipelineHandler.h"
#include "../response/HttpResponse.h"
#include "../util/UriView.h"
#include "IHttpContextHandlerFactory.h"

#include <any>
#include <map>
#include <string>
#include <string_view>

namespace HttpServerAdvanced
{   
    class HttpContextAccess;

    class HttpContext
    {
    public:
        HttpContext(HttpServerAdvanced::HttpServerBase &server, IHttpContextHandlerFactory& handlerFactory)
            : server_(server),
              method_(), version_(), url_(), headers_(),
              remoteAddress_(), remotePort_(0), localAddress_(), localPort_(0),
              handlerFactory_(handlerFactory)
        {
        }


    private:
        HttpServerAdvanced::HttpServerBase &server_;
        const HttpContextPhaseFlags *completedPhases_ = nullptr;
        mutable std::map<std::string, std::any> items_;

        // Retains the legacy HTTP activation data carried through the pipeline.
        std::string method_;
        std::string version_;
        std::string url_;
        HttpHeaderCollection headers_;
        std::string remoteAddress_;
        uint16_t remotePort_;
        std::string localAddress_;
        uint16_t localPort_;
        IHttpContextHandlerFactory& handlerFactory_;

        void invalidateTextCaches()
        {
            cachedUriView_.reset();
        }

        void setRequestLine(const char *method, uint16_t versionMajor, uint16_t versionMinor, std::string_view url)
        {
            method_ = method != nullptr ? method : "";
            version_ = std::to_string(versionMajor) + "." + std::to_string(versionMinor);
            url_.assign(url.data(), url.size());
            invalidateTextCaches();
        }

        void setRequestAddresses(std::string_view remoteAddress, uint16_t remotePort, std::string_view localAddress, uint16_t localPort)
        {
            remoteAddress_.assign(remoteAddress.data(), remoteAddress.size());
            remotePort_ = remotePort;
            localAddress_.assign(localAddress.data(), localAddress.size());
            localPort_ = localPort;
        }

        void setHeader(std::string_view field, std::string_view value)
        {
            headers_.set(HttpHeader(field, value));
        }

        void bindCompletedPhases(const HttpContextPhaseFlags *completedPhases)
        {
            completedPhases_ = completedPhases;
        }

        std::unique_ptr<IHttpHandler> createHandler()
        {
            return handlerFactory_.create(*this);
        }

        mutable std::unique_ptr<UriView> cachedUriView_;

        friend class DeferredRegistryHandler;
        friend class HttpContextAccess;

    public:
        ~HttpContext() = default;

        inline HttpServerAdvanced::HttpServerBase &server() { return server_; }
        inline HttpContextPhaseFlags completedPhases() const { return completedPhases_ != nullptr ? *completedPhases_ : 0; }

        // Legacy request accessors preserved on the final context type.
        inline std::string_view version() const { return versionView(); }
        inline std::string_view versionView() const { return std::string_view(version_.data(), version_.size()); }
        inline const char *method() const { return method_.c_str(); }
        inline std::string_view methodView() const { return std::string_view(method_.data(), method_.size()); }
        inline std::string_view url() const { return urlView(); }
        inline std::string_view urlView() const { return std::string_view(url_.data(), url_.size()); }
        inline const HttpHeaderCollection &headers() const { return headers_; }
        inline std::string_view remoteAddress() const { return std::string_view(remoteAddress_.data(), remoteAddress_.size()); }
        inline uint16_t remotePort() const { return remotePort_; }
        inline std::string_view localAddress() const { return std::string_view(localAddress_.data(), localAddress_.size()); }
        inline uint16_t localPort() const { return localPort_; }
        inline std::map<std::string, std::any> &items() const{
            return items_;
        }
        inline std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string body)
        {
            return handlerFactory_.createResponse(status, std::move(body));
        }

        UriView &uriView() const
        {
            if (!cachedUriView_)
            {
                cachedUriView_ = std::make_unique<UriView>(url_);
            }
            return *cachedUriView_;
        }

    public:
        static PipelineHandlerPtr createPipelineHandler(HttpServerAdvanced::HttpServerBase &server, IHttpContextHandlerFactory& handlerFactory);

        friend class DeferredRegistryHandler;
    };

}




