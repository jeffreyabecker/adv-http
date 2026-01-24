#pragma once
#include <Arduino.h>
#include <vector>
#include <optional>
#include <algorithm>
#include "./HttpHeader.h"
#include "./HttpHeaderCollection.h"
#include "./StringUtility.h"

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

    void set(const HttpHeader &header, bool forceOverwrite = false);

    void set(HttpHeader &&header, bool forceOverwrite = true);

    void set(const String &name, const String &value, bool forceOverwrite = false);

    void remove(const String &name);

    void remove(const String &name, const String &value);
  };

} // namespace HttpServerAdvanced
