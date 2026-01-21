#pragma once
// https://github.com/nodejs/llhttp
#include <Arduino.h>
#include <http_parser.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <algorithm>
#include "../util/Util.h"

namespace HttpServerAdvanced::Core
{

  class HttpHeader
  {
  private:
    String name_;
    String value_;

  public:
    HttpHeader();
    explicit HttpHeader(const String &name, const String &value) : name_(name), value_(value) {}
    explicit HttpHeader(const char *name, const char *value) : name_(name), value_(value) {}
    explicit HttpHeader(const char *name, const String &value) : name_(name), value_(value) {}
    explicit HttpHeader(const String &name, const char *value) : name_(name), value_(value) {}

    ~HttpHeader();
    HttpHeader(const HttpHeader &) = default;
    HttpHeader(HttpHeader &&) noexcept = default;
    HttpHeader &operator=(const HttpHeader &) = default;
    HttpHeader &operator=(HttpHeader &&) noexcept = default;

    const String &name() const
    {
      return name_;
    }
    void setName(const String &name)
    {
      name_ = name;
    }
    const String &value() const
    {
      return value_;
    }
    void setValue(const String &value)
    {
      value_ = value;
    }

#if __cpp_structured_bindings
    // Enable structured bindings (C++17 and later)
    friend auto tie(const HttpHeader &header)
    {
      return std::tie(header.name_, header.value_);
    }
#endif

    static std::vector<std::pair<String, std::optional<String>>> parseDirectives(const String &val = "");
    static constexpr const char *WwwAuthenticate = "WWW-Authenticate";
    static constexpr const char *Authorization = "Authorization";
    static constexpr const char *ProxyAuthenticate = "Proxy-Authenticate";
    static constexpr const char *ProxyAuthorization = "Proxy-Authorization";
    static constexpr const char *Age = "Age";
    static constexpr const char *CacheControl = "Cache-Control";
    static constexpr const char *ClearSiteData = "Clear-Site-Data";
    static constexpr const char *Expires = "Expires";
    static constexpr const char *LastModified = "Last-Modified";
    static constexpr const char *ETag = "ETag";
    static constexpr const char *IfMatch = "If-Match";
    static constexpr const char *IfNoneMatch = "If-None-Match";
    static constexpr const char *IfModifiedSince = "If-Modified-Since";
    static constexpr const char *IfUnmodifiedSince = "If-Unmodified-Since";
    static constexpr const char *Vary = "Vary";
    static constexpr const char *Connection = "Connection";
    static constexpr const char *KeepAlive = "Keep-Alive";
    static constexpr const char *Accept = "Accept";
    static constexpr const char *AcceptEncoding = "Accept-Encoding";
    static constexpr const char *AcceptLanguage = "Accept-Language";
    static constexpr const char *AcceptPatch = "Accept-Patch";
    static constexpr const char *AcceptPost = "Accept-Post";
    static constexpr const char *Expect = "Expect";
    static constexpr const char *MaxForwards = "Max-Forwards";
    static constexpr const char *Cookie = "Cookie";
    static constexpr const char *SetCookie = "Set-Cookie";
    static constexpr const char *AccessControlAllowCredentials = "Access-Control-Allow-Credentials";
    static constexpr const char *AccessControlAllowHeaders = "Access-Control-Allow-Headers";
    static constexpr const char *AccessControlAllowMethods = "Access-Control-Allow-Methods";
    static constexpr const char *AccessControlAllowOrigin = "Access-Control-Allow-Origin";
    static constexpr const char *AccessControlExposeHeaders = "Access-Control-Expose-Headers";
    static constexpr const char *AccessControlMaxAge = "Access-Control-Max-Age";
    static constexpr const char *AccessControlRequestHeaders = "Access-Control-Request-Headers";
    static constexpr const char *AccessControlRequestMethod = "Access-Control-Request-Method";
    static constexpr const char *Origin = "Origin";
    static constexpr const char *TimingAllowOrigin = "Timing-Allow-Origin";
    static constexpr const char *ContentDisposition = "Content-Disposition";
    static constexpr const char *IntegrityPolicy = "Integrity-Policy";
    static constexpr const char *IntegrityPolicyReportOnly = "Integrity-Policy-Report-Only";
    static constexpr const char *ContentLength = "Content-Length";
    static constexpr const char *ContentType = "Content-Type";
    static constexpr const char *ContentEncoding = "Content-Encoding";
    static constexpr const char *ContentLanguage = "Content-Language";
    static constexpr const char *ContentLocation = "Content-Location";
    static constexpr const char *Prefer = "Prefer";
    static constexpr const char *PreferenceApplied = "Preference-Applied";
    static constexpr const char *Forwarded = "Forwarded";
    static constexpr const char *Via = "Via";
    static constexpr const char *AcceptRanges = "Accept-Ranges";
    static constexpr const char *Range = "Range";
    static constexpr const char *IfRange = "If-Range";
    static constexpr const char *ContentRange = "Content-Range";
    static constexpr const char *Location = "Location";
    static constexpr const char *Refresh = "Refresh";
    static constexpr const char *From = "From";
    static constexpr const char *Host = "Host";
    static constexpr const char *Referer = "Referer";
    static constexpr const char *ReferrerPolicy = "Referrer-Policy";
    static constexpr const char *UserAgent = "User-Agent";
    static constexpr const char *Allow = "Allow";
    static constexpr const char *Server = "Server";
    static constexpr const char *CrossOriginEmbedderPolicy = "Cross-Origin-Embedder-Policy";
    static constexpr const char *CROSS_ORIGIN_OPENER_POLICY = "Cross-Origin-Opener-Policy";
    static constexpr const char *CROSS_ORIGIN_RESOURCE_POLICY = "Cross-Origin-Resource-Policy";
    static constexpr const char *CONTENT_SECURITY_POLICY = "Content-Security-Policy";
    static constexpr const char *ContentSecurityPolicyReportOnly = "Content-Security-Policy-Report-Only";
    static constexpr const char *PermissionsPolicy = "Permissions-Policy";
    static constexpr const char *StrictTransportSecurity = "Strict-Transport-Security";
    static constexpr const char *UpgradeInsecureRequests = "Upgrade-Insecure-Requests";
    static constexpr const char *XContentTypeOptions = "X-Content-Type-Options";
    static constexpr const char *XFrameOptions = "X-Frame-Options";
    static constexpr const char *XPermittedCrossDomainPolicies = "X-Permitted-Cross-Domain-Policies";
    static constexpr const char *XPoweredBy = "X-Powered-By";
    static constexpr const char *XXssProtection = "X-XSS-Protection";
    static constexpr const char *SecFetchSite = "Sec-Fetch-Site";
    static constexpr const char *SecFetchMode = "Sec-Fetch-Mode";
    static constexpr const char *SecFetchUser = "Sec-Fetch-User";
    static constexpr const char *SecFetchDest = "Sec-Fetch-Dest";
    static constexpr const char *SecPurpose = "Sec-Purpose";
    static constexpr const char *ServiceWorkerNavigationPreload = "Service-Worker-Navigation-Preload";
    static constexpr const char *ReportingEndpoints = "Reporting-Endpoints";
    static constexpr const char *TransferEncoding = "Transfer-Encoding";
    static constexpr const char *Te = "TE";
    static constexpr const char *Trailer = "Trailer";
    static constexpr const char *SecWebSocketAccept = "Sec-WebSocket-Accept";
    static constexpr const char *SecWebSocketExtensions = "Sec-WebSocket-Extensions";
    static constexpr const char *SecWebSocketKey = "Sec-WebSocket-Key";
    static constexpr const char *SecWebSocketProtocol = "Sec-WebSocket-Protocol";
    static constexpr const char *SecWebSocketVersion = "Sec-WebSocket-Version";
    static constexpr const char *AltSvc = "Alt-Svc";
    static constexpr const char *AltUsed = "Alt-Used";
    static constexpr const char *Date = "Date";
    static constexpr const char *Link = "Link";
    static constexpr const char *RetryAfter = "Retry-After";
    static constexpr const char *ServerTiming = "Server-Timing";
    static constexpr const char *ServiceWorker = "Service-Worker";
    static constexpr const char *ServiceWorkerAllowed = "Service-Worker-Allowed";
    static constexpr const char *SourceMap = "SourceMap";
    static constexpr const char *Upgrade = "Upgrade";
    static constexpr const char *Priority = "Priority";
    static constexpr const char *AttributionReportingEligible = "Attribution-Reporting-Eligible";
    static constexpr const char *AttributionReportingRegisterSource = "Attribution-Reporting-Register-Source";
    static constexpr const char *AttributionReportingRegisterTrigger = "Attribution-Reporting-Register-Trigger";
    static constexpr const char *AcceptCh = "Accept-CH";
    static constexpr const char *DeviceMemory = "Device-Memory";
    static constexpr const char *AcceptCharset = "Accept-Charset";
    static constexpr const char *Pragma = "Pragma";
    static constexpr const char *XForwardedFor = "X-Forwarded-For";
  };

  class HttpHeadersCollection : public std::vector<HttpHeader>
  {

  public:
    HttpHeadersCollection() = default;

    inline HttpHeadersCollection(std::initializer_list<HttpHeader> headers)
        : HttpHeadersCollection()
    {
      for (const auto &h : headers)
      {
        set(h);
      }
    }

    inline HttpHeadersCollection(std::initializer_list<std::pair<const String &, const String &>> headers)
    {
      for (const auto &h : headers)
      {
        set(h.first, h.second);
      }
    }

    inline std::optional<HttpHeader> find(const String &name) const
    {
      auto it = std::find_if(begin(), end(), [&name](const HttpHeader &header)
                             { return header.name().equalsIgnoreCase(name); });
      if (it != end())
      {
        return *it;
      }
      return std::nullopt;
    }

    inline bool exists(const String &name) const
    {
      auto it = find(name);
      return it.has_value();
    }

    inline bool exists(const String &name, const String &value) const
    {
      auto it = std::find_if(begin(), end(), [&name, &value](const HttpHeader &header)
                             { return header.name().equalsIgnoreCase(name) && header.value().equals(value); });
      return it != end();
    }

    inline void set(const HttpHeader &header, bool forceOverwrite = false)
    {
      set(header.name(), header.value(), forceOverwrite);
    }

    inline void set(HttpHeader &&header, bool forceOverwrite = false)
    {
      static const char *const duplicable[] = {
          HttpHeader::SetCookie,
          HttpHeader::WwwAuthenticate};
      static const char *const consolidatable[] = {
          HttpHeader::Accept,
          HttpHeader::AcceptCharset,
          HttpHeader::AcceptEncoding,
          HttpHeader::AcceptLanguage,
          HttpHeader::CacheControl,
          HttpHeader::Connection,
          HttpHeader::ContentDisposition,
          HttpHeader::ContentEncoding,
          HttpHeader::ContentLanguage,
          HttpHeader::Vary,
          HttpHeader::Pragma,
          HttpHeader::XForwardedFor};
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

    inline void set(const String &name, const String &value, bool forceOverwrite = false)
    {
      HttpHeader header(name, value);
      set(std::move(header), forceOverwrite);
    }

    inline void remove(const String &name)
    {
      erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                           { return h.name().equalsIgnoreCase(name); }),
            end());
    }

    inline void remove(const String &name, const String &value)
    {
      erase(std::remove_if(begin(), end(), [&](const HttpHeader &h)
                           { return h.name().equalsIgnoreCase(name) && h.value().equals(value); }),
            end());
    }
  };

}