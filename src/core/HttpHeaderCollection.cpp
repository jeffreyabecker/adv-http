#include "HttpHeaderCollection.h"
#include "HttpHeader.h"
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
                               { return StringUtil::compareTo(h.nameView(), header.nameView(), true) == 0; });

        if (it != end())
        {
            if (StringUtil::compareTo(header.nameView(), HttpHeaderNames::SetCookie, true) == 0)
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
                    String mergedValue = it->value();
                    mergedValue += ",";
                    mergedValue += value;
                    *it = HttpHeader(name, std::move(mergedValue));
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
                             { return StringUtil::compareTo(h.nameView(), std::string_view(name.c_str(), name.length()), true) == 0; }),
              end());
    }

    void HttpHeaderCollection::remove(const String &name, const String &value)
    {
        erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                             {
                                 return StringUtil::compareTo(h.nameView(), std::string_view(name.c_str(), name.length()), true) == 0 &&
                                        h.valueView() == std::string_view(value.c_str(), value.length());
                             }),
              end());
    }
} // namespace HttpServerAdvanced
