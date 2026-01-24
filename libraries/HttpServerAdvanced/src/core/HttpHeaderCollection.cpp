#include "./HttpHeaderCollection.h"
#include "./HttpHeader.h"
#include "../util/StringUtility.h"

namespace HttpServerAdvanced
{
    void HttpHeaderCollection::set(const HttpHeader &header, bool forceOverwrite)
    {
        set(header.name(), header.value(), forceOverwrite);
    }

    void HttpHeaderCollection::set(HttpHeader &&header, bool forceOverwrite)
    {
        const String &name = header.name();
        const String &value = header.value();

        auto it = std::find_if(begin(), end(), [&](const HttpHeader &h)
                               { return h.name().equalsIgnoreCase(name); });

        if (it != end())
        {
            if (name.equalsIgnoreCase(HttpHeaderNames::SetCookie))
            {
                if (forceOverwrite)
                {
                    *it = std::move(header);
                }
                else
                {
                    emplace_back(std::move(header));
                }
                return;
            }
            else
            {
                if (forceOverwrite)
                {
                    *it = std::move(header);
                }
                else
                {
                    *it = HttpHeader(name, it->value() + "," + value);
                }
            }
        }
        else
        {
            emplace_back(std::move(header));
        }
    }

    void HttpHeaderCollection::set(const String &name, const String &value, bool forceOverwrite)
    {
        HttpHeader header(name, value);
        set(std::move(header), forceOverwrite);
    }

    void HttpHeaderCollection::remove(const String &name)
    {
        erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                             { return h.name().equalsIgnoreCase(name); }),
              end());
    }

    void HttpHeaderCollection::remove(const String &name, const String &value)
    {
        erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                             { return h.name().equalsIgnoreCase(name) && h.value().equals(value); }),
              end());
    }
} // namespace HttpServerAdvanced
