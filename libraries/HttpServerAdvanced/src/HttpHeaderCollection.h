#pragma once
#include <Arduino.h>
#include <vector>
#include <optional>
#include <algorithm>
#include "./HttpHeader.h"
#include "./HttpHeaderCollection.h"

namespace HttpServerAdvanced
{
  class HttpHeaderCollection : public std::vector<HttpHeader>
  {

  public:
    HttpHeaderCollection() = default;

    HttpHeaderCollection(std::initializer_list<HttpHeader> headers)
        : HttpHeaderCollection()
    {
      for (const auto &h : headers)
      {
        set(h);
      }
    }

    HttpHeaderCollection(std::initializer_list<std::pair<const String &, const String &>> headers)
    {
      for (const auto &h : headers)
      {
        set(h.first, h.second);
      }
    }

    // Accept initializer lists of C-style string pairs to avoid ambiguity when using braced pairs
    HttpHeaderCollection(std::initializer_list<std::pair<const char *, const char *>> headers)
    {
      for (const auto &h : headers)
      {
        set(h.first, h.second);
      }
    }

    std::optional<HttpHeader> find(const String &name) const
    {
      auto it = std::find_if(begin(), end(), [&name](const HttpHeader &header)
                             { return header.name().equalsIgnoreCase(name); });
      if (it != end())
      {
        return *it;
      }
      return std::nullopt;
    }

    bool exists(const String &name) const
    {
      auto it = find(name);
      return it.has_value();
    }

    bool exists(const String &name, const String &value) const
    {
      auto it = std::find_if(begin(), end(), [&name, &value](const HttpHeader &header)
                             { return header.name().equalsIgnoreCase(name) && header.value().equals(value); });
      return it != end();
    }

    void set(const HttpHeader &header, bool forceOverwrite = false)
    {
      set(header.name(), header.value(), forceOverwrite);
    }

    void set(HttpHeader &&header, bool forceOverwrite = true)
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

    void set(const String &name, const String &value, bool forceOverwrite = false)
    {
      HttpHeader header(name, value);
      set(std::move(header), forceOverwrite);
    }

    void remove(const String &name)
    {
      erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                           { return h.name().equalsIgnoreCase(name); }),
            end());
    }

    void remove(const String &name, const String &value)
    {
      erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                           { return h.name().equalsIgnoreCase(name) && h.value().equals(value); }),
            end());
    }
  };

} // namespace HttpServerAdvanced
