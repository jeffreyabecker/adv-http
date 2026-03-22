#include "HttpHeaderCollection.h"
#include "HttpHeader.h"

namespace HttpServerAdvanced
{
    void HttpHeaderCollection::set(const HttpHeader &header, bool forceOverwrite)
    {
        set(header.nameView(), header.valueView(), forceOverwrite);
    }

    void HttpHeaderCollection::set(HttpHeader &&header, bool forceOverwrite)
    {
        const std::string_view name = header.nameView();
        const std::string_view value = header.valueView();

        auto it = std::find_if(begin(), end(), [&](const HttpHeader &h)
                               { return HttpHeaderCollectionDetail::EqualsIgnoreCase(h.nameView(), header.nameView()); });

        if (it != end())
        {
            if (HttpHeaderCollectionDetail::EqualsIgnoreCase(header.nameView(), HttpHeaderNames::SetCookie))
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
                    std::string mergedValue(it->valueView());
                    mergedValue += ",";
                    mergedValue.append(value.data(), value.size());
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
        set(std::string_view(name.c_str(), name.length()), std::string_view(value.c_str(), value.length()), forceOverwrite);
    }

    void HttpHeaderCollection::remove(const String &name)
    {
        remove(std::string_view(name.c_str(), name.length()));
    }

    void HttpHeaderCollection::remove(const String &name, const String &value)
    {
        remove(std::string_view(name.c_str(), name.length()), std::string_view(value.c_str(), value.length()));
    }
} // namespace HttpServerAdvanced
