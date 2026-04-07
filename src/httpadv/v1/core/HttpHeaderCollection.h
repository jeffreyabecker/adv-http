#pragma once
#include <vector>
#include <optional>
#include <algorithm>
#include <cctype>
#include <string_view>
#include "HttpHeader.h"

namespace lumalink::http::core
{
  namespace HttpHeaderCollectionDetail
  {
    inline char ToLowerAscii(char value)
    {
      return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
    }

    inline bool EqualsIgnoreCase(std::string_view left, std::string_view right)
    {
      return left.size() == right.size() &&
             std::equal(left.begin(), left.end(), right.begin(), [](char lhs, char rhs)
                        { return ToLowerAscii(lhs) == ToLowerAscii(rhs); });
    }
  }

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

    // Accept initializer lists of C-style string pairs to avoid ambiguity when using braced pairs
    HttpHeaderCollection(std::initializer_list<std::pair<const char *, const char *>> headers)
    {
      for (const auto &h : headers)
      {
        set(std::string_view(h.first != nullptr ? h.first : ""), std::string_view(h.second != nullptr ? h.second : ""));
      }
    }

    std::optional<HttpHeader> find(const char *name) const
    {
      return find(std::string_view(name != nullptr ? name : ""));
    }

    std::optional<HttpHeader> find(std::string_view name) const
    {
      auto it = std::find_if(begin(), end(), [name](const HttpHeader &header)
                             { return HttpHeaderCollectionDetail::EqualsIgnoreCase(header.nameView(), name); });
      if (it != end())
      {
        return *it;
      }
      return std::nullopt;
    }

    bool exists(const char *name) const
    {
      return exists(std::string_view(name != nullptr ? name : ""));
    }

    bool exists(std::string_view name) const
    {
      return find(name).has_value();
    }

    bool exists(const char *name, const char *value) const
    {
      return exists(std::string_view(name != nullptr ? name : ""), std::string_view(value != nullptr ? value : ""));
    }

    bool exists(std::string_view name, std::string_view value) const
    {
      auto it = std::find_if(begin(), end(), [name, value](const HttpHeader &header)
                             {
                               return HttpHeaderCollectionDetail::EqualsIgnoreCase(header.nameView(), name) &&
                                      header.valueView() == value;
                             });
      return it != end();
    }

    void set(const HttpHeader &header, bool forceOverwrite = false);

    void set(HttpHeader &&header, bool forceOverwrite = true);

    void set(const char *name, const char *value, bool forceOverwrite = false)
    {
      set(std::string_view(name != nullptr ? name : ""), std::string_view(value != nullptr ? value : ""), forceOverwrite);
    }

    void set(std::string_view name, std::string_view value, bool forceOverwrite = false)
    {
      set(HttpHeader(name, value), forceOverwrite);
    }

    void remove(const char *name)
    {
      remove(std::string_view(name != nullptr ? name : ""));
    }

    void remove(std::string_view name)
    {
      erase(std::remove_if(begin(), end(), [name](const HttpHeader &h)
                           { return HttpHeaderCollectionDetail::EqualsIgnoreCase(h.nameView(), name); }),
            end());
    }

    void remove(const char *name, const char *value)
    {
      remove(std::string_view(name != nullptr ? name : ""), std::string_view(value != nullptr ? value : ""));
    }

    void remove(std::string_view name, std::string_view value)
    {
      erase(std::remove_if(begin(), end(), [name, value](const HttpHeader &h)
                           {
                             return HttpHeaderCollectionDetail::EqualsIgnoreCase(h.nameView(), name) &&
                                    h.valueView() == value;
                           }),
            end());
    }
  };

} // namespace lumalink::http::core
