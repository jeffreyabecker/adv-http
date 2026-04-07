#pragma once

#include <string>
#include <string_view>

#include "HttpUtility.h"

namespace lumalink::http::util
{
    class UriView
    {
    public:
        UriView();
        explicit UriView(std::string uri);
        explicit UriView(std::string_view uri);
        explicit UriView(const char *uri);
        UriView(const UriView &that);
        UriView &operator=(const UriView &that);

        std::string_view scheme() const { return scheme_; }
        std::string_view userinfo() const { return userinfo_; }
        std::string_view host() const { return host_; }
        std::string_view port() const { return port_; }
        std::string_view path() const { return path_; }
        std::string_view query() const { return query_; }
        const WebUtility::QueryParameters &queryView() const { return queryView_; }
        std::string_view fragment() const { return fragment_; }
        const std::string &uri() const { return uri_; }

    private:
        std::string uri_;
        std::string_view scheme_;
        std::string_view userinfo_;
        std::string_view host_;
        std::string_view port_;
        std::string_view path_;
        std::string_view query_;
        WebUtility::QueryParameters queryView_;
        std::string_view fragment_;

        void parse();
    };

} // namespace lumalink::http::util