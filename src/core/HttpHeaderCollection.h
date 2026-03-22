#pragma once
#include <Arduino.h>
#include <vector>
#include <optional>
#include <algorithm>
#include <string_view>
#include "HttpHeader.h"
#include "../util/StringUtility.h"

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
        set(std::string_view(h.first.c_str(), h.first.length()), std::string_view(h.second.c_str(), h.second.length()));
      }
    }

    // Accept initializer lists of C-style string pairs to avoid ambiguity when using braced pairs
    HttpHeaderCollection(std::initializer_list<std::pair<const char *, const char *>> headers)
    {
      for (const auto &h : headers)
      {
        set(std::string_view(h.first != nullptr ? h.first : ""), std::string_view(h.second != nullptr ? h.second : ""));
      }
    }

    std::optional<HttpHeader> find(const String &name) const
    {
      return find(std::string_view(name.c_str(), name.length()));
    }

    std::optional<HttpHeader> find(std::string_view name) const
    {
      auto it = std::find_if(begin(), end(), [name](const HttpHeader &header)
                             { return StringUtil::compareTo(header.nameView(), name, true) == 0; });
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

    bool exists(const char *name) const
    {
      return exists(std::string_view(name != nullptr ? name : ""));
    }

    bool exists(std::string_view name) const
    {
      return find(name).has_value();
    }

    bool exists(const String &name, const String &value) const
    {
      return exists(std::string_view(name.c_str(), name.length()), std::string_view(value.c_str(), value.length()));
    }

    bool exists(const char *name, const char *value) const
    {
      return exists(std::string_view(name != nullptr ? name : ""), std::string_view(value != nullptr ? value : ""));
    }

    bool exists(std::string_view name, std::string_view value) const
    {
      auto it = std::find_if(begin(), end(), [name, value](const HttpHeader &header)
                             {
                               return StringUtil::compareTo(header.nameView(), name, true) == 0 &&
                                      header.valueView() == value;
                             });
      return it != end();
    }

    void set(const HttpHeader &header, bool forceOverwrite = false);

    void set(HttpHeader &&header, bool forceOverwrite = true);

    void set(const String &name, const String &value, bool forceOverwrite = false);

    void set(const char *name, const char *value, bool forceOverwrite = false)
    {
      set(std::string_view(name != nullptr ? name : ""), std::string_view(value != nullptr ? value : ""), forceOverwrite);
    }

    void set(std::string_view name, std::string_view value, bool forceOverwrite = false)
    {
      set(HttpHeader(name, value), forceOverwrite);
    }

    void remove(const String &name);

    void remove(const char *name)
    {
      remove(std::string_view(name != nullptr ? name : ""));
    }

    void remove(std::string_view name)
    {
      erase(std::remove_if(begin(), end(), [name](const HttpHeader &h)
                           { return StringUtil::compareTo(h.nameView(), name, true) == 0; }),
            end());
    }

    void remove(const String &name, const String &value);

    void remove(const char *name, const char *value)
    {
      remove(std::string_view(name != nullptr ? name : ""), std::string_view(value != nullptr ? value : ""));
    }

    void remove(std::string_view name, std::string_view value)
    {
      erase(std::remove_if(begin(), end(), [name, value](const HttpHeader &h)
                           {
                             return StringUtil::compareTo(h.nameView(), name, true) == 0 &&
                                    h.valueView() == value;
                           }),
            end());
    }
  };

} // namespace HttpServerAdvanced
