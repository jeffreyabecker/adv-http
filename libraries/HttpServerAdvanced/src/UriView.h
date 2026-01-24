#pragma once
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <optional>
#include "./HttpUtility.h"
#include "./KeyValuePairView.h"
#include "./StringView.h"
namespace HttpServerAdvanced
{

    class UriView
    {
    public:
        UriView(const String &uri = "")
            : uri_(uri)
        {
            parse();
        }
        UriView(const UriView &that)
            : uri_(that.uri_), scheme_(that.scheme_), userinfo_(that.userinfo_), host_(that.host_), port_(that.port_), path_(that.path_), _query(that._query), queryView_(that.queryView_), fragment_(that.fragment_) {}

        const StringView &scheme() const { return scheme_; }
        const StringView &userinfo() const { return userinfo_; }
        const StringView &host() const { return host_; }
        const StringView &port() const { return port_; }
        const StringView &path() const { return path_; }
        const StringView &query() const { return _query; }
        const KeyValuePairView<String, String> &queryView() const { return queryView_; }
        const StringView &fragment() const { return fragment_; }

    private:
        String uri_;  // Store original URI for StringView lifetime
        StringView scheme_;
        StringView userinfo_;
        StringView host_;
        StringView port_;
        StringView path_;
        StringView _query;
        KeyValuePairView<String, String> queryView_;
        StringView fragment_;

        void parse();
    };

} // namespace ExtendedHttp