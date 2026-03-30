#include "UriView.h"

#include "HttpUtility.h"

namespace httpadv::v1::util
{
    UriView::UriView()
        : UriView(std::string())
    {
    }

    UriView::UriView(std::string uri)
        : uri_(std::move(uri))
    {
        parse();
    }

    UriView::UriView(std::string_view uri)
        : UriView(std::string(uri))
    {
    }

    UriView::UriView(const char *uri)
        : UriView(uri == nullptr ? std::string() : std::string(uri))
    {
    }

    UriView::UriView(const UriView &that)
        : uri_(that.uri_), queryView_(that.queryView_)
    {
        parse();
    }

    UriView &UriView::operator=(const UriView &that)
    {
        if (this != &that)
        {
            uri_ = that.uri_;
            queryView_ = that.queryView_;
            parse();
        }
        return *this;
    }

    void UriView::parse()
    {
        scheme_ = std::string_view();
        userinfo_ = std::string_view();
        host_ = std::string_view();
        port_ = std::string_view();
        path_ = std::string_view();
        query_ = std::string_view();
        queryView_ = WebUtility::QueryParameters();
        fragment_ = std::string_view();

        const char *data = uri_.c_str();
        const std::string_view uriView(uri_);
        std::size_t pos = 0;
        const std::size_t len = uriView.size();

        const std::size_t schemeEnd = uriView.find(':');
        const std::size_t slashslash = uriView.find("//");
        if (schemeEnd != std::string_view::npos && (slashslash == std::string_view::npos || schemeEnd < slashslash))
        {
            scheme_ = std::string_view(data, schemeEnd);
            pos = schemeEnd + 1;
        }

        if (pos + 2 <= len && uriView[pos] == '/' && uriView[pos + 1] == '/')
        {
            pos += 2;
            const std::size_t authorityEnd = uriView.find('/', pos);
            const std::size_t queryStart = uriView.find('?', pos);
            const std::size_t fragmentStart = uriView.find('#', pos);

            std::size_t end = len;
            if (authorityEnd != std::string_view::npos && authorityEnd < end)
                end = authorityEnd;
            if (queryStart != std::string_view::npos && queryStart < end)
                end = queryStart;
            if (fragmentStart != std::string_view::npos && fragmentStart < end)
                end = fragmentStart;

            const std::size_t atPos = uriView.find('@', pos);
            std::size_t hostport_start = pos;
            if (atPos != std::string_view::npos && atPos < end)
            {
                userinfo_ = std::string_view(data + pos, atPos - pos);
                hostport_start = atPos + 1;
            }

            const std::size_t hostportEnd = end;
            const std::size_t portSep = uriView.find(':', hostport_start);
            if (portSep != std::string_view::npos && portSep < hostportEnd)
            {
                host_ = std::string_view(data + hostport_start, portSep - hostport_start);
                port_ = std::string_view(data + portSep + 1, hostportEnd - portSep - 1);
            }
            else
            {
                host_ = std::string_view(data + hostport_start, hostportEnd - hostport_start);
            }
            pos = end;
        }

        if (pos < len && uriView[pos] == '/')
        {
            const std::size_t pathEnd = uriView.find('?', pos);
            const std::size_t fragmentStart = uriView.find('#', pos);
            std::size_t end = len;
            if (pathEnd != std::string_view::npos && pathEnd < end)
                end = pathEnd;
            if (fragmentStart != std::string_view::npos && fragmentStart < end)
                end = fragmentStart;
            path_ = std::string_view(data + pos, end - pos);
            pos = end;
        }

        if (pos < len && uriView[pos] == '?')
        {
            const std::size_t queryEnd = uriView.find('#', pos + 1);
            if (queryEnd != std::string_view::npos)
            {
                query_ = std::string_view(data + pos + 1, queryEnd - pos - 1);
                queryView_ = WebUtility::ParseQueryParameters(query_);
                pos = queryEnd;
            }
            else
            {
                query_ = std::string_view(data + pos + 1, len - pos - 1);
                queryView_ = WebUtility::ParseQueryParameters(query_);
                pos = len;
            }
        }

        if (pos < len && uriView[pos] == '#')
        {
            fragment_ = std::string_view(data + pos + 1, len - pos - 1);
        }
    }
} // namespace HttpServerAdvanced
