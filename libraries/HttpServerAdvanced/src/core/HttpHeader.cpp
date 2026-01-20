#include "./HttpHeader.h"

#include <memory>
namespace HttpServerAdvanced::Core
{

    HttpHeader::HttpHeader() {}

    HttpHeader::~HttpHeader() {}

    std::vector<std::pair<String, std::optional<String>>> HttpHeader::parseDirectives(const String &val)
    {
        std::vector<std::pair<String, std::optional<String>>> directives;
        if (val.isEmpty())
        {
            return directives;
        }

        auto parseSingleDirective = [&](const String &directive)
        {
            int eqPos = directive.indexOf('=');
            if (eqPos == -1)
            {
                directives.emplace_back(directive, std::nullopt);
            }
            else
            {
                String key = directive.substring(0, eqPos);
                String value = directive.substring(eqPos + 1);
                key.trim();
                value.trim();
                directives.emplace_back(key, value);
            }
        };

        // Split by comma
        int start = 0;
        int end = val.indexOf(',');
        while (end != -1)
        {
            String directive = val.substring(start, end);
            directive.trim();
            parseSingleDirective(directive);
            start = end + 1;
            end = val.indexOf(',', start);
        }
        // Last one
        String directive = val.substring(start);
        directive.trim();
        parseSingleDirective(directive);

        return directives;
    }

    HttpHeadersCollection::HttpHeadersCollection(std::initializer_list<HttpHeader> headers)
        : HttpHeadersCollection()
    {
        for (const auto &h : headers)
        {
            set(h);
        }
    }

    HttpHeadersCollection::HttpHeadersCollection(std::initializer_list<std::pair<const String &, const String &>> headers)
    {
        for (const auto &h : headers)
        {
            set(h.first, h.second);
        }
    }

    std::optional<HttpHeader> HttpHeadersCollection::find(const String &name) const
    {
        auto it = std::find_if(begin(), end(), [&name](const HttpHeader &header)
                               { return header.name().equalsIgnoreCase(name); });
        if (it != end())
        {
            return *it;
        }
        return std::nullopt;
    }

    bool HttpHeadersCollection::exists(const String &name) const
    {
        auto it = find(name);
        return it.has_value();
    }

    void HttpHeadersCollection::set(const HttpHeader &header, bool forceOverwrite)
    {
        set(header.name(), header.value(), forceOverwrite);
    }

    void HttpHeadersCollection::set(HttpHeader &&header, bool forceOverwrite)
    {
        static const char *const duplicable[] = {
            HttpHeaders::SET_COOKIE,
            HttpHeaders::WWW_AUTHENTICATE};
        static const char *const consolidatable[] = {
            HttpHeaders::ACCEPT,
            HttpHeaders::ACCEPT_CHARSET,
            HttpHeaders::ACCEPT_ENCODING,
            HttpHeaders::ACCEPT_LANGUAGE,
            HttpHeaders::CACHE_CONTROL,
            HttpHeaders::CONNECTION,
            HttpHeaders::CONTENT_DISPOSITION,
            HttpHeaders::CONTENT_ENCODING,
            HttpHeaders::CONTENT_LANGUAGE,
            HttpHeaders::VARY,
            HttpHeaders::PRAGMA,
            HttpHeaders::X_FORWARDED_FOR};

        auto is_in = [](const String &name, const char *const *list, std::size_t count)
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                if (name.equalsIgnoreCase(list[i]))
                    return true;
            }
            return false;
        };

        const String &name = header.name();
        const String &value = header.value();

        if (is_in(name, duplicable, sizeof(duplicable) / sizeof(duplicable[0])))
        {
            if (forceOverwrite)
            {
                erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                                     { return h.name().equalsIgnoreCase(name); }),
                      end());
            }
            emplace_back(std::move(header));
            return;
        }

        auto it = std::find_if(begin(), end(), [&](const HttpHeader &h)
                               { return h.name().equalsIgnoreCase(name); });

        if (is_in(name, consolidatable, sizeof(consolidatable) / sizeof(consolidatable[0])))
        {
            if (it != end())
            {
                if (forceOverwrite)
                {
                    *it = std::move(header);
                }
                else
                {
                    it->setValue(it->value() + "," + value);
                }
            }
            else
            {
                emplace_back(std::move(header));
            }
            return;
        }

        if (it != end())
        {
            if (forceOverwrite)
            {
                *it = std::move(header);
            }
            else
            {
                it->setValue(value);
            }
        }
        else
        {
            emplace_back(std::move(header));
        }
    }

    void HttpHeadersCollection::set(const String &name, const String &value, bool forceOverwrite)
    {
        HttpHeader header(name, value);
        set(std::move(header), forceOverwrite);
    }

    void HttpHeadersCollection::remove(const String &name)
    {
        erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                             { return h.name().equalsIgnoreCase(name); }),
              end());
    }
    void HttpHeadersCollection::remove(const String& name, const String &value)
    {
        erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                             { return h.name().equalsIgnoreCase(name) && h.value().equals(value); }),
              end());
    }
    bool HttpHeadersCollection::exists(const String& name, const String &value) const
    {
        auto it = std::find_if(begin(), end(), [&name, &value](const HttpHeader &header)
                               { return header.name().equalsIgnoreCase(name) && header.value().equals(value); });
        return it != end();
    }

}