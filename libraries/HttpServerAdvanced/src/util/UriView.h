#pragma once
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <optional>
#include "../util/Util.h"
#include "../HttpUtility.h"
namespace HttpServerAdvanced
{
    template <typename TKey, typename TValue>
    class KeyValuePairView
    {
    protected:
        std::vector<std::pair<TKey, TValue>> pairs_;

    public:
        KeyValuePairView() : pairs_() {}
        KeyValuePairView(const std::vector<std::pair<TKey, TValue>> &pairs)
            : pairs_(pairs) {}
        KeyValuePairView(const KeyValuePairView &that) : pairs_(that.pairs_) {}
        KeyValuePairView(KeyValuePairView &&that) : pairs_(std::move(that.pairs_)) {}
        KeyValuePairView &operator=(const KeyValuePairView &that)
        {
            pairs_ = that.pairs_;
            return *this;
        }
        KeyValuePairView &operator=(KeyValuePairView &&that)
        {
            pairs_ = std::move(that.pairs_);
            return *this;
        }
        virtual std::size_t exists(const TKey &key) const
        {
            std::size_t cnt = 0;
            for (const auto &kv : pairs_)
            {
                if (kv.first == key)
                    ++cnt;
            }
            return cnt;
        }
        // Gets the value for a key, or returns std::nullopt if not found
        std::optional<TValue> get(const TKey &key) const
        {
            for (const auto &kv : pairs_)
            {
                if (kv.first == key)
                    return kv.second;
            }
            return std::nullopt;
        }
        std::vector<TValue> getAll(const TKey &key) const
        {
            std::vector<TValue> values;
            for (const auto &kv : pairs_)
            {
                if (kv.first == key)
                    values.push_back(kv.second);
            }
            return values;
        }

        const std::vector<std::pair<TKey, TValue>> &pairs() const
        {
            return pairs_;
        }
    };

    class UriView
    {
    public:
        UriView(const StringView &uri = "")
        {
            parse(uri);
        }
        UriView(const UriView &that)
            : scheme_(that.scheme_), userinfo_(that.userinfo_), host_(that.host_), port_(that.port_), path_(that.path_), _query(that._query), queryView_(that.queryView_), fragment_(that.fragment_) {}

        const StringView &scheme() const { return scheme_; }
        const StringView &userinfo() const { return userinfo_; }
        const StringView &host() const { return host_; }
        const StringView &port() const { return port_; }
        const StringView &path() const { return path_; }
        const StringView &query() const { return _query; }
        const KeyValuePairView<String, String> &queryView() const { return queryView_; }
        const StringView &fragment() const { return fragment_; }

    private:
        StringView scheme_;
        StringView userinfo_;
        StringView host_;
        StringView port_;
        StringView path_;
        StringView _query;
        KeyValuePairView<String, String> queryView_;
        StringView fragment_;

        void parse(const StringView &uri)
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
                    queryView_ = KeyValuePairView<String, String>(HttpUtility::parseQueryString(_query));
                    pos = query_end;
                }
                else
                {
                    _query = uri.substring(pos + 1);
                    queryView_ = KeyValuePairView<String, String>(HttpUtility::parseQueryString(_query));
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