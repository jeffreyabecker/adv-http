#pragma once
// https://github.com/nodejs/llhttp
#include <Arduino.h>
#include <http_parser.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <algorithm>


namespace HttpServerAdvanced
{

  struct HttpHeaderNames{
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
  class HttpHeader
  {
  private:
    String name_;
    String value_;

  public:
    HttpHeader();
    explicit HttpHeader(const String &name, const String &value) : name_(name), value_(value) {}
    explicit HttpHeader(const char *name, const char *value) : name_(name), value_(value) {}
    explicit HttpHeader(const char *name, String &&value) : name_(name), value_(std::move(value)) {}
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
    static HttpHeader&& WwwAuthenticate(const String& value) { return HttpHeader(HttpHeaderNames::WwwAuthenticate, value); }
    static HttpHeader&& Authorization(const String& value) { return HttpHeader(HttpHeaderNames::Authorization, value); }
    static HttpHeader&& ProxyAuthenticate(const String& value) { return HttpHeader(HttpHeaderNames::ProxyAuthenticate, value); }
    static HttpHeader&& ProxyAuthorization(const String& value) { return HttpHeader(HttpHeaderNames::ProxyAuthorization, value); }
    static HttpHeader&& Age(const String& value) { return HttpHeader(HttpHeaderNames::Age, value); }
    static HttpHeader&& CacheControl(const String& value) { return HttpHeader(HttpHeaderNames::CacheControl, value); }
    static HttpHeader&& ClearSiteData(const String& value) { return HttpHeader(HttpHeaderNames::ClearSiteData, value); }
    static HttpHeader&& Expires(const String& value) { return HttpHeader(HttpHeaderNames::Expires, value); }
    static HttpHeader&& LastModified(const String& value) { return HttpHeader(HttpHeaderNames::LastModified, value); }
    static HttpHeader&& ETag(const String& value) { return HttpHeader(HttpHeaderNames::ETag, value); }
    static HttpHeader&& IfMatch(const String& value) { return HttpHeader(HttpHeaderNames::IfMatch, value); }
    static HttpHeader&& IfNoneMatch(const String& value) { return HttpHeader(HttpHeaderNames::IfNoneMatch, value); }
    static HttpHeader&& IfModifiedSince(const String& value) { return HttpHeader(HttpHeaderNames::IfModifiedSince, value); }
    static HttpHeader&& IfUnmodifiedSince(const String& value) { return HttpHeader(HttpHeaderNames::IfUnmodifiedSince, value); }
    static HttpHeader&& Vary(const String& value) { return HttpHeader(HttpHeaderNames::Vary, value); }
    static HttpHeader&& Connection(const String& value) { return HttpHeader(HttpHeaderNames::Connection, value); }
    static HttpHeader&& KeepAlive(const String& value) { return HttpHeader(HttpHeaderNames::KeepAlive, value); }
    static HttpHeader&& Accept(const String& value) { return HttpHeader(HttpHeaderNames::Accept, value); }
    static HttpHeader&& AcceptEncoding(const String& value) { return HttpHeader(HttpHeaderNames::AcceptEncoding, value); }
    static HttpHeader&& AcceptLanguage(const String& value) { return HttpHeader(HttpHeaderNames::AcceptLanguage, value); }
    static HttpHeader&& AcceptPatch(const String& value) { return HttpHeader(HttpHeaderNames::AcceptPatch, value); }
    static HttpHeader&& AcceptPost(const String& value) { return HttpHeader(HttpHeaderNames::AcceptPost, value); }
    static HttpHeader&& Expect(const String& value) { return HttpHeader(HttpHeaderNames::Expect, value); }
    static HttpHeader&& MaxForwards(const String& value) { return HttpHeader(HttpHeaderNames::MaxForwards, value); }
    static HttpHeader&& Cookie(const String& value) { return HttpHeader(HttpHeaderNames::Cookie, value); }
    static HttpHeader&& SetCookie(const String& value) { return HttpHeader(HttpHeaderNames::SetCookie, value); }
    static HttpHeader&& AccessControlAllowCredentials(const String& value) { return HttpHeader(HttpHeaderNames::AccessControlAllowCredentials, value); }
    static HttpHeader&& AccessControlAllowHeaders(const String& value) { return HttpHeader(HttpHeaderNames::AccessControlAllowHeaders, value); }
    static HttpHeader&& AccessControlAllowMethods(const String& value) { return HttpHeader(HttpHeaderNames::AccessControlAllowMethods, value); }
    static HttpHeader&& AccessControlAllowOrigin(const String& value) { return HttpHeader(HttpHeaderNames::AccessControlAllowOrigin, value); }
    static HttpHeader&& AccessControlExposeHeaders(const String& value) { return HttpHeader(HttpHeaderNames::AccessControlExposeHeaders, value); }
    static HttpHeader&& AccessControlMaxAge(const String& value) { return HttpHeader(HttpHeaderNames::AccessControlMaxAge, value); }
    static HttpHeader&& AccessControlRequestHeaders(const String& value) { return HttpHeader(HttpHeaderNames::AccessControlRequestHeaders, value); }
    static HttpHeader&& AccessControlRequestMethod(const String& value) { return HttpHeader(HttpHeaderNames::AccessControlRequestMethod, value); }
    static HttpHeader&& Origin(const String& value) { return HttpHeader(HttpHeaderNames::Origin, value); }
    static HttpHeader&& TimingAllowOrigin(const String& value) { return HttpHeader(HttpHeaderNames::TimingAllowOrigin, value); }
    static HttpHeader&& ContentDisposition(const String& value) { return HttpHeader(HttpHeaderNames::ContentDisposition, value); }
    static HttpHeader&& IntegrityPolicy(const String& value) { return HttpHeader(HttpHeaderNames::IntegrityPolicy, value); }
    static HttpHeader&& IntegrityPolicyReportOnly(const String& value) { return HttpHeader(HttpHeaderNames::IntegrityPolicyReportOnly, value); }
    static HttpHeader&& ContentLength(const String& value) { return HttpHeader(HttpHeaderNames::ContentLength, value); }
    static HttpHeader&& ContentType(const String& value) { return HttpHeader(HttpHeaderNames::ContentType, value); }
    static HttpHeader&& ContentEncoding(const String& value) { return HttpHeader(HttpHeaderNames::ContentEncoding, value); }
    static HttpHeader&& ContentLanguage(const String& value) { return HttpHeader(HttpHeaderNames::ContentLanguage, value); }
    static HttpHeader&& ContentLocation(const String& value) { return HttpHeader(HttpHeaderNames::ContentLocation, value); }
    static HttpHeader&& Prefer(const String& value) { return HttpHeader(HttpHeaderNames::Prefer, value); }
    static HttpHeader&& PreferenceApplied(const String& value) { return HttpHeader(HttpHeaderNames::PreferenceApplied, value); }
    static HttpHeader&& Forwarded(const String& value) { return HttpHeader(HttpHeaderNames::Forwarded, value); }
    static HttpHeader&& Via(const String& value) { return HttpHeader(HttpHeaderNames::Via, value); }
    static HttpHeader&& AcceptRanges(const String& value) { return HttpHeader(HttpHeaderNames::AcceptRanges, value); }
    static HttpHeader&& Range(const String& value) { return HttpHeader(HttpHeaderNames::Range, value); }
    static HttpHeader&& IfRange(const String& value) { return HttpHeader(HttpHeaderNames::IfRange, value); }
    static HttpHeader&& ContentRange(const String& value) { return HttpHeader(HttpHeaderNames::ContentRange, value); }
    static HttpHeader&& Location(const String& value) { return HttpHeader(HttpHeaderNames::Location, value); }
    static HttpHeader&& Refresh(const String& value) { return HttpHeader(HttpHeaderNames::Refresh, value); }
    static HttpHeader&& From(const String& value) { return HttpHeader(HttpHeaderNames::From, value); }
    static HttpHeader&& Host(const String& value) { return HttpHeader(HttpHeaderNames::Host, value); }
    static HttpHeader&& Referer(const String& value) { return HttpHeader(HttpHeaderNames::Referer, value); }
    static HttpHeader&& ReferrerPolicy(const String& value) { return HttpHeader(HttpHeaderNames::ReferrerPolicy, value); }
    static HttpHeader&& UserAgent(const String& value) { return HttpHeader(HttpHeaderNames::UserAgent, value); }
    static HttpHeader&& Allow(const String& value) { return HttpHeader(HttpHeaderNames::Allow, value); }
    static HttpHeader&& Server(const String& value) { return HttpHeader(HttpHeaderNames::Server, value); }
    static HttpHeader&& CrossOriginEmbedderPolicy(const String& value) { return HttpHeader(HttpHeaderNames::CrossOriginEmbedderPolicy, value); }
    static HttpHeader&& CrossOriginOpenerPolicy(const String& value) { return HttpHeader(HttpHeaderNames::CROSS_ORIGIN_OPENER_POLICY, value); }
    static HttpHeader&& CrossOriginResourcePolicy(const String& value) { return HttpHeader(HttpHeaderNames::CROSS_ORIGIN_RESOURCE_POLICY, value); }
    static HttpHeader&& ContentSecurityPolicy(const String& value) { return HttpHeader(HttpHeaderNames::CONTENT_SECURITY_POLICY, value); }
    static HttpHeader&& ContentSecurityPolicyReportOnly(const String& value) { return HttpHeader(HttpHeaderNames::ContentSecurityPolicyReportOnly, value); }
    static HttpHeader&& PermissionsPolicy(const String& value) { return HttpHeader(HttpHeaderNames::PermissionsPolicy, value); }
    static HttpHeader&& StrictTransportSecurity(const String& value) { return HttpHeader(HttpHeaderNames::StrictTransportSecurity, value); }
    static HttpHeader&& UpgradeInsecureRequests(const String& value) { return HttpHeader(HttpHeaderNames::UpgradeInsecureRequests, value); }
    static HttpHeader&& XContentTypeOptions(const String& value) { return HttpHeader(HttpHeaderNames::XContentTypeOptions, value); }
    static HttpHeader&& XFrameOptions(const String& value) { return HttpHeader(HttpHeaderNames::XFrameOptions, value); }
    static HttpHeader&& XPermittedCrossDomainPolicies(const String& value) { return HttpHeader(HttpHeaderNames::XPermittedCrossDomainPolicies, value); }
    static HttpHeader&& XPoweredBy(const String& value) { return HttpHeader(HttpHeaderNames::XPoweredBy, value); }
    static HttpHeader&& XXssProtection(const String& value) { return HttpHeader(HttpHeaderNames::XXssProtection, value); }
    static HttpHeader&& SecFetchSite(const String& value) { return HttpHeader(HttpHeaderNames::SecFetchSite, value); }
    static HttpHeader&& SecFetchMode(const String& value) { return HttpHeader(HttpHeaderNames::SecFetchMode, value); }
    static HttpHeader&& SecFetchUser(const String& value) { return HttpHeader(HttpHeaderNames::SecFetchUser, value); }
    static HttpHeader&& SecFetchDest(const String& value) { return HttpHeader(HttpHeaderNames::SecFetchDest, value); }
    static HttpHeader&& SecPurpose(const String& value) { return HttpHeader(HttpHeaderNames::SecPurpose, value); }
    static HttpHeader&& ServiceWorkerNavigationPreload(const String& value) { return HttpHeader(HttpHeaderNames::ServiceWorkerNavigationPreload, value); }
    static HttpHeader&& ReportingEndpoints(const String& value) { return HttpHeader(HttpHeaderNames::ReportingEndpoints, value); }
    static HttpHeader&& TransferEncoding(const String& value) { return HttpHeader(HttpHeaderNames::TransferEncoding, value); }
    static HttpHeader&& Te(const String& value) { return HttpHeader(HttpHeaderNames::Te, value); }
    static HttpHeader&& Trailer(const String& value) { return HttpHeader(HttpHeaderNames::Trailer, value); }
    static HttpHeader&& SecWebSocketAccept(const String& value) { return HttpHeader(HttpHeaderNames::SecWebSocketAccept, value); }
    static HttpHeader&& SecWebSocketExtensions(const String& value) { return HttpHeader(HttpHeaderNames::SecWebSocketExtensions, value); }
    static HttpHeader&& SecWebSocketKey(const String& value) { return HttpHeader(HttpHeaderNames::SecWebSocketKey, value); }
    static HttpHeader&& SecWebSocketProtocol(const String& value) { return HttpHeader(HttpHeaderNames::SecWebSocketProtocol, value); }
    static HttpHeader&& SecWebSocketVersion(const String& value) { return HttpHeader(HttpHeaderNames::SecWebSocketVersion, value); }
    static HttpHeader&& AltSvc(const String& value) { return HttpHeader(HttpHeaderNames::AltSvc, value); }
    static HttpHeader&& AltUsed(const String& value) { return HttpHeader(HttpHeaderNames::AltUsed, value); }
    static HttpHeader&& Date(const String& value) { return HttpHeader(HttpHeaderNames::Date, value); }
    static HttpHeader&& Link(const String& value) { return HttpHeader(HttpHeaderNames::Link, value); }
    static HttpHeader&& RetryAfter(const String& value) { return HttpHeader(HttpHeaderNames::RetryAfter, value); }
    static HttpHeader&& ServerTiming(const String& value) { return HttpHeader(HttpHeaderNames::ServerTiming, value); }
    static HttpHeader&& ServiceWorker(const String& value) { return HttpHeader(HttpHeaderNames::ServiceWorker, value); }
    static HttpHeader&& ServiceWorkerAllowed(const String& value) { return HttpHeader(HttpHeaderNames::ServiceWorkerAllowed, value); }
    static HttpHeader&& SourceMap(const String& value) { return HttpHeader(HttpHeaderNames::SourceMap, value); }
    static HttpHeader&& Upgrade(const String& value) { return HttpHeader(HttpHeaderNames::Upgrade, value); }
    static HttpHeader&& Priority(const String& value) { return HttpHeader(HttpHeaderNames::Priority, value); }
    static HttpHeader&& AttributionReportingEligible(const String& value) { return HttpHeader(HttpHeaderNames::AttributionReportingEligible, value); }
    static HttpHeader&& AttributionReportingRegisterSource(const String& value) { return HttpHeader(HttpHeaderNames::AttributionReportingRegisterSource, value); }
    static HttpHeader&& AttributionReportingRegisterTrigger(const String& value) { return HttpHeader(HttpHeaderNames::AttributionReportingRegisterTrigger, value); }
    static HttpHeader&& AcceptCh(const String& value) { return HttpHeader(HttpHeaderNames::AcceptCh, value); }
    static HttpHeader&& DeviceMemory(const String& value) { return HttpHeader(HttpHeaderNames::DeviceMemory, value); }
    static HttpHeader&& AcceptCharset(const String& value) { return HttpHeader(HttpHeaderNames::AcceptCharset, value); }
    static HttpHeader&& Pragma(const String& value) { return HttpHeader(HttpHeaderNames::Pragma, value); }
    static HttpHeader&& XForwardedFor(const String& value) { return HttpHeader(HttpHeaderNames::XForwardedFor, value); }

  };

  class HttpHeadersCollection : public std::vector<HttpHeader>
  {

  public:
    HttpHeadersCollection() = default;

    HttpHeadersCollection(std::initializer_list<HttpHeader> headers)
        : HttpHeadersCollection()
    {
      for (const auto &h : headers)
      {
        set(h);
      }
    }

    HttpHeadersCollection(std::initializer_list<std::pair<const String &, const String &>> headers)
    {
      for (const auto &h : headers)
      {
        set(h.first, h.second);
      }
    }

    // Accept initializer lists of C-style string pairs to avoid ambiguity when using braced pairs
    HttpHeadersCollection(std::initializer_list<std::pair<const char *, const char *>> headers)
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

    void set(HttpHeader &&header, bool forceOverwrite = false)
    {
      static const char *const duplicable[] = {
          HttpHeaderNames::SetCookie,
          HttpHeaderNames::WwwAuthenticate};
      static const char *const consolidatable[] = {
          HttpHeaderNames::Accept,
          HttpHeaderNames::AcceptCharset,
          HttpHeaderNames::AcceptEncoding,
          HttpHeaderNames::AcceptLanguage,
          HttpHeaderNames::CacheControl,
          HttpHeaderNames::Connection,
          HttpHeaderNames::ContentDisposition,
          HttpHeaderNames::ContentEncoding,
          HttpHeaderNames::ContentLanguage,
          HttpHeaderNames::Vary,
          HttpHeaderNames::Pragma,
          HttpHeaderNames::XForwardedFor};
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

}