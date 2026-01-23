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

        void parse()
        {
            const char* data = uri_.c_str();
            std::size_t pos = 0;
            std::size_t len = uri_.length();

            // Parse scheme
            std::size_t scheme_end = uri_.indexOf(':');
            std::size_t slashslash = uri_.indexOf("//");
            if (scheme_end != -1 && (slashslash == -1 || scheme_end < slashslash))
            {
                scheme_ = StringView(data, scheme_end);
                pos = scheme_end + 1;
            }

            // Parse authority (userinfo@host[:port])
            if (pos + 2 <= len && uri_[pos] == '/' && uri_[pos + 1] == '/')
            {
                pos += 2;
                std::size_t authority_end = uri_.indexOf('/', pos);
                std::size_t query_start = uri_.indexOf('?', pos);
                std::size_t fragment_start = uri_.indexOf('#', pos);

                std::size_t end = len;
                if (authority_end != -1 && authority_end < end)
                    end = authority_end;
                if (query_start != -1 && query_start < end)
                    end = query_start;
                if (fragment_start != -1 && fragment_start < end)
                    end = fragment_start;

                std::size_t at_pos = uri_.indexOf('@', pos);
                std::size_t hostport_start = pos;
                if (at_pos != -1 && at_pos < end)
                {
                    userinfo_ = StringView(data + pos, at_pos - pos);
                    hostport_start = at_pos + 1;
                }

                std::size_t hostport_end = end;
                std::size_t port_sep = uri_.indexOf(':', hostport_start);
                if (port_sep != -1 && port_sep < hostport_end)
                {
                    host_ = StringView(data + hostport_start, port_sep - hostport_start);
                    port_ = StringView(data + port_sep + 1, hostport_end - port_sep - 1);
                }
                else
                {
                    host_ = StringView(data + hostport_start, hostport_end - hostport_start);
                }
                pos = end;
            }

            // Parse path
            if (pos < len && uri_[pos] == '/')
            {
                std::size_t path_end = uri_.indexOf('?', pos);
                std::size_t fragment_start = uri_.indexOf('#', pos);
                std::size_t end = len;
                if (path_end != -1 && path_end < end)
                    end = path_end;
                if (fragment_start != -1 && fragment_start < end)
                    end = fragment_start;
                path_ = StringView(data + pos, end - pos);
                pos = end;
            }

            // Parse query
            if (pos < len && uri_[pos] == '?')
            {
                std::size_t query_end = uri_.indexOf('#', pos + 1);
                if (query_end != -1)
                {
                    _query = StringView(data + pos + 1, query_end - pos - 1);
                    queryView_ = KeyValuePairView<String, String>(WebUtility::ParseQueryString(_query.begin(), _query.length()));
                    pos = query_end;
                }
                else
                {
                    _query = StringView(data + pos + 1, len - pos - 1);
                    queryView_ = KeyValuePairView<String, String>(WebUtility::ParseQueryString(_query.begin(), _query.length()));
                    pos = len;
                }
            }

            // Parse fragment
            if (pos < len && uri_[pos] == '#')
            {
                fragment_ = StringView(data + pos + 1, len - pos - 1);
            }
        }
    };

} // namespace ExtendedHttp