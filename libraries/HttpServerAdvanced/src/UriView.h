#pragma once
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <optional>
#include "./HttpUtility.h"
#include "./KeyValuePairView.h"
namespace HttpServerAdvanced
{

    class UriView
    {
    public:
        UriView(const String &uri = "")
        {
            parse(uri);
        }
        UriView(const UriView &that)
            : scheme_(that.scheme_), userinfo_(that.userinfo_), host_(that.host_), port_(that.port_), path_(that.path_), _query(that._query), queryView_(that.queryView_), fragment_(that.fragment_) {}

        const String &scheme() const { return scheme_; }
        const String &userinfo() const { return userinfo_; }
        const String &host() const { return host_; }
        const String &port() const { return port_; }
        const String &path() const { return path_; }
        const String &query() const { return _query; }
        const KeyValuePairView<String, String> &queryView() const { return queryView_; }
        const String &fragment() const { return fragment_; }

    private:
        String scheme_;
        String userinfo_;
        String host_;
        String port_;
        String path_;
        String _query;
        KeyValuePairView<String, String> queryView_;
        String fragment_;

        void parse(const String &uri)
        {
            std::size_t pos = 0;
            std::size_t len = uri.length();

            // Parse scheme
            std::size_t scheme_end = uri.indexOf(':');
            std::size_t slashslash = uri.indexOf("//");
            if (scheme_end != -1 && (slashslash == -1 || scheme_end < slashslash))
            {
                scheme_ = uri.substring(0, scheme_end);
                pos = scheme_end + 1;
            }

            // Parse authority (userinfo@host[:port])
            if (uri.substring(pos, pos + 2) == "//")
            {
                pos += 2;
                std::size_t authority_end = uri.indexOf('/', pos);
                std::size_t query_start = uri.indexOf('?', pos);
                std::size_t fragment_start = uri.indexOf('#', pos);

                std::size_t end = len;
                if (authority_end != -1 && authority_end < end)
                    end = authority_end;
                if (query_start != -1 && query_start < end)
                    end = query_start;
                if (fragment_start != -1 && fragment_start < end)
                    end = fragment_start;

                std::size_t at_pos = uri.indexOf('@', pos);
                std::size_t hostport_start = pos;
                if (at_pos != -1 && at_pos < end)
                {
                    userinfo_ = uri.substring(pos, at_pos);
                    hostport_start = at_pos + 1;
                }

                std::size_t hostport_end = end;
                std::size_t port_sep = uri.indexOf(':', hostport_start);
                if (port_sep != -1 && port_sep < hostport_end)
                {
                    host_ = uri.substring(hostport_start, port_sep);
                    port_ = uri.substring(port_sep + 1, hostport_end);
                }
                else
                {
                    host_ = uri.substring(hostport_start, hostport_end);
                }
                pos = end;
            }

            // Parse path
            if (pos < len && uri[pos] == '/')
            {
                std::size_t path_end = uri.indexOf('?', pos);
                std::size_t fragment_start = uri.indexOf('#', pos);
                std::size_t end = len;
                if (path_end != -1 && path_end < end)
                    end = path_end;
                if (fragment_start != -1 && fragment_start < end)
                    end = fragment_start;
                path_ = uri.substring(pos, end);
                pos = end;
            }

            // Parse query
            if (pos < len && uri[pos] == '?')
            {
                std::size_t query_end = uri.indexOf('#', pos + 1);
                if (query_end != -1)
                {
                    _query = uri.substring(pos + 1, query_end);
                    queryView_ = KeyValuePairView<String, String>(HttpUtility::ParseQueryString(_query));
                    pos = query_end;
                }
                else
                {
                    _query = uri.substring(pos + 1);
                    queryView_ = KeyValuePairView<String, String>(HttpUtility::ParseQueryString(_query));
                    pos = len;
                }
            }

            // Parse fragment
            if (pos < len && uri[pos] == '#')
            {
                fragment_ = uri.substring(pos + 1);
            }
        }
    };

} // namespace ExtendedHttp