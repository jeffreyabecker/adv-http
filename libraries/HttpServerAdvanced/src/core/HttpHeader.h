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

  struct HttpHeaderNames
  {
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
    HttpHeader() = default;
    explicit HttpHeader(const String &name, const String &value) : name_(name), value_(value) {}
    explicit HttpHeader(const char *name, const char *value) : name_(name), value_(value) {}
    explicit HttpHeader(const char *name, String &&value) : name_(name), value_(std::move(value)) {}
    explicit HttpHeader(const String &name, const char *value) : name_(name), value_(value) {}

    ~HttpHeader() = default;
    HttpHeader(const HttpHeader &) = default;
    HttpHeader(HttpHeader &&) noexcept = default;
    HttpHeader &operator=(const HttpHeader &) = default;
    HttpHeader &operator=(HttpHeader &&) noexcept = default;

    const String &name() const
    {
      return name_;
    }

    const String &value() const
    {
      return value_;
    }

#if __cpp_structured_bindings
    // Enable structured bindings (C++17 and later)
    friend auto tie(const HttpHeader &header)
    {
      return std::tie(header.name_, header.value_);
    }
#endif

    static std::vector<std::pair<String, std::optional<String>>> parseDirectives(const String &val = "");
    static HttpHeader WwwAuthenticate(String &&value) { return HttpHeader(HttpHeaderNames::WwwAuthenticate, std::move(value)); }
    static HttpHeader Authorization(String &&value) { return HttpHeader(HttpHeaderNames::Authorization, std::move(value)); }
    static HttpHeader ProxyAuthenticate(String &&value) { return HttpHeader(HttpHeaderNames::ProxyAuthenticate, std::move(value)); }
    static HttpHeader ProxyAuthorization(String &&value) { return HttpHeader(HttpHeaderNames::ProxyAuthorization, std::move(value)); }
    static HttpHeader Age(String &&value) { return HttpHeader(HttpHeaderNames::Age, std::move(value)); }
    static HttpHeader CacheControl(String &&value) { return HttpHeader(HttpHeaderNames::CacheControl, std::move(value)); }
    static HttpHeader ClearSiteData(String &&value) { return HttpHeader(HttpHeaderNames::ClearSiteData, std::move(value)); }
    static HttpHeader Expires(String &&value) { return HttpHeader(HttpHeaderNames::Expires, std::move(value)); }
    static HttpHeader LastModified(String &&value) { return HttpHeader(HttpHeaderNames::LastModified, std::move(value)); }
    static HttpHeader ETag(String &&value) { return HttpHeader(HttpHeaderNames::ETag, std::move(value)); }
    static HttpHeader IfMatch(String &&value) { return HttpHeader(HttpHeaderNames::IfMatch, std::move(value)); }
    static HttpHeader IfNoneMatch(String &&value) { return HttpHeader(HttpHeaderNames::IfNoneMatch, std::move(value)); }
    static HttpHeader IfModifiedSince(String &&value) { return HttpHeader(HttpHeaderNames::IfModifiedSince, std::move(value)); }
    static HttpHeader IfUnmodifiedSince(String &&value) { return HttpHeader(HttpHeaderNames::IfUnmodifiedSince, std::move(value)); }
    static HttpHeader Vary(String &&value) { return HttpHeader(HttpHeaderNames::Vary, std::move(value)); }
    static HttpHeader Connection(String &&value) { return HttpHeader(HttpHeaderNames::Connection, std::move(value)); }
    static HttpHeader KeepAlive(String &&value) { return HttpHeader(HttpHeaderNames::KeepAlive, std::move(value)); }
    static HttpHeader Accept(String &&value) { return HttpHeader(HttpHeaderNames::Accept, std::move(value)); }
    static HttpHeader AcceptEncoding(String &&value) { return HttpHeader(HttpHeaderNames::AcceptEncoding, std::move(value)); }
    static HttpHeader AcceptLanguage(String &&value) { return HttpHeader(HttpHeaderNames::AcceptLanguage, std::move(value)); }
    static HttpHeader AcceptPatch(String &&value) { return HttpHeader(HttpHeaderNames::AcceptPatch, std::move(value)); }
    static HttpHeader AcceptPost(String &&value) { return HttpHeader(HttpHeaderNames::AcceptPost, std::move(value)); }
    static HttpHeader Expect(String &&value) { return HttpHeader(HttpHeaderNames::Expect, std::move(value)); }
    static HttpHeader MaxForwards(String &&value) { return HttpHeader(HttpHeaderNames::MaxForwards, std::move(value)); }
    static HttpHeader Cookie(String &&value) { return HttpHeader(HttpHeaderNames::Cookie, std::move(value)); }
    static HttpHeader SetCookie(String &&value) { return HttpHeader(HttpHeaderNames::SetCookie, std::move(value)); }
    static HttpHeader AccessControlAllowCredentials(String &&value) { return HttpHeader(HttpHeaderNames::AccessControlAllowCredentials, std::move(value)); }
    static HttpHeader AccessControlAllowHeaders(String &&value) { return HttpHeader(HttpHeaderNames::AccessControlAllowHeaders, std::move(value)); }
    static HttpHeader AccessControlAllowMethods(String &&value) { return HttpHeader(HttpHeaderNames::AccessControlAllowMethods, std::move(value)); }
    static HttpHeader AccessControlAllowOrigin(String &&value) { return HttpHeader(HttpHeaderNames::AccessControlAllowOrigin, std::move(value)); }
    static HttpHeader AccessControlExposeHeaders(String &&value) { return HttpHeader(HttpHeaderNames::AccessControlExposeHeaders, std::move(value)); }
    static HttpHeader AccessControlMaxAge(String &&value) { return HttpHeader(HttpHeaderNames::AccessControlMaxAge, std::move(value)); }
    static HttpHeader AccessControlRequestHeaders(String &&value) { return HttpHeader(HttpHeaderNames::AccessControlRequestHeaders, std::move(value)); }
    static HttpHeader AccessControlRequestMethod(String &&value) { return HttpHeader(HttpHeaderNames::AccessControlRequestMethod, std::move(value)); }
    static HttpHeader Origin(String &&value) { return HttpHeader(HttpHeaderNames::Origin, std::move(value)); }
    static HttpHeader TimingAllowOrigin(String &&value) { return HttpHeader(HttpHeaderNames::TimingAllowOrigin, std::move(value)); }
    static HttpHeader ContentDisposition(String &&value) { return HttpHeader(HttpHeaderNames::ContentDisposition, std::move(value)); }
    static HttpHeader IntegrityPolicy(String &&value) { return HttpHeader(HttpHeaderNames::IntegrityPolicy, std::move(value)); }
    static HttpHeader IntegrityPolicyReportOnly(String &&value) { return HttpHeader(HttpHeaderNames::IntegrityPolicyReportOnly, std::move(value)); }
    static HttpHeader ContentLength(String &&value) { return HttpHeader(HttpHeaderNames::ContentLength, std::move(value)); }
    static HttpHeader ContentType(String &&value) { return HttpHeader(HttpHeaderNames::ContentType, std::move(value)); }
    static HttpHeader ContentEncoding(String &&value) { return HttpHeader(HttpHeaderNames::ContentEncoding, std::move(value)); }
    static HttpHeader ContentLanguage(String &&value) { return HttpHeader(HttpHeaderNames::ContentLanguage, std::move(value)); }
    static HttpHeader ContentLocation(String &&value) { return HttpHeader(HttpHeaderNames::ContentLocation, std::move(value)); }
    static HttpHeader Prefer(String &&value) { return HttpHeader(HttpHeaderNames::Prefer, std::move(value)); }
    static HttpHeader PreferenceApplied(String &&value) { return HttpHeader(HttpHeaderNames::PreferenceApplied, std::move(value)); }
    static HttpHeader Forwarded(String &&value) { return HttpHeader(HttpHeaderNames::Forwarded, std::move(value)); }
    static HttpHeader Via(String &&value) { return HttpHeader(HttpHeaderNames::Via, std::move(value)); }
    static HttpHeader AcceptRanges(String &&value) { return HttpHeader(HttpHeaderNames::AcceptRanges, std::move(value)); }
    static HttpHeader Range(String &&value) { return HttpHeader(HttpHeaderNames::Range, std::move(value)); }
    static HttpHeader IfRange(String &&value) { return HttpHeader(HttpHeaderNames::IfRange, std::move(value)); }
    static HttpHeader ContentRange(String &&value) { return HttpHeader(HttpHeaderNames::ContentRange, std::move(value)); }
    static HttpHeader Location(String &&value) { return HttpHeader(HttpHeaderNames::Location, std::move(value)); }
    static HttpHeader Refresh(String &&value) { return HttpHeader(HttpHeaderNames::Refresh, std::move(value)); }
    static HttpHeader From(String &&value) { return HttpHeader(HttpHeaderNames::From, std::move(value)); }
    static HttpHeader Host(String &&value) { return HttpHeader(HttpHeaderNames::Host, std::move(value)); }
    static HttpHeader Referer(String &&value) { return HttpHeader(HttpHeaderNames::Referer, std::move(value)); }
    static HttpHeader ReferrerPolicy(String &&value) { return HttpHeader(HttpHeaderNames::ReferrerPolicy, std::move(value)); }
    static HttpHeader UserAgent(String &&value) { return HttpHeader(HttpHeaderNames::UserAgent, std::move(value)); }
    static HttpHeader Allow(String &&value) { return HttpHeader(HttpHeaderNames::Allow, std::move(value)); }
    static HttpHeader Server(String &&value) { return HttpHeader(HttpHeaderNames::Server, std::move(value)); }
    static HttpHeader CrossOriginEmbedderPolicy(String &&value) { return HttpHeader(HttpHeaderNames::CrossOriginEmbedderPolicy, std::move(value)); }
    static HttpHeader CrossOriginOpenerPolicy(String &&value) { return HttpHeader(HttpHeaderNames::CROSS_ORIGIN_OPENER_POLICY, std::move(value)); }
    static HttpHeader CrossOriginResourcePolicy(String &&value) { return HttpHeader(HttpHeaderNames::CROSS_ORIGIN_RESOURCE_POLICY, std::move(value)); }
    static HttpHeader ContentSecurityPolicy(String &&value) { return HttpHeader(HttpHeaderNames::CONTENT_SECURITY_POLICY, std::move(value)); }
    static HttpHeader ContentSecurityPolicyReportOnly(String &&value) { return HttpHeader(HttpHeaderNames::ContentSecurityPolicyReportOnly, std::move(value)); }
    static HttpHeader PermissionsPolicy(String &&value) { return HttpHeader(HttpHeaderNames::PermissionsPolicy, std::move(value)); }
    static HttpHeader StrictTransportSecurity(String &&value) { return HttpHeader(HttpHeaderNames::StrictTransportSecurity, std::move(value)); }
    static HttpHeader UpgradeInsecureRequests(String &&value) { return HttpHeader(HttpHeaderNames::UpgradeInsecureRequests, std::move(value)); }
    static HttpHeader XContentTypeOptions(String &&value) { return HttpHeader(HttpHeaderNames::XContentTypeOptions, std::move(value)); }
    static HttpHeader XFrameOptions(String &&value) { return HttpHeader(HttpHeaderNames::XFrameOptions, std::move(value)); }
    static HttpHeader XPermittedCrossDomainPolicies(String &&value) { return HttpHeader(HttpHeaderNames::XPermittedCrossDomainPolicies, std::move(value)); }
    static HttpHeader XPoweredBy(String &&value) { return HttpHeader(HttpHeaderNames::XPoweredBy, std::move(value)); }
    static HttpHeader XXssProtection(String &&value) { return HttpHeader(HttpHeaderNames::XXssProtection, std::move(value)); }
    static HttpHeader SecFetchSite(String &&value) { return HttpHeader(HttpHeaderNames::SecFetchSite, std::move(value)); }
    static HttpHeader SecFetchMode(String &&value) { return HttpHeader(HttpHeaderNames::SecFetchMode, std::move(value)); }
    static HttpHeader SecFetchUser(String &&value) { return HttpHeader(HttpHeaderNames::SecFetchUser, std::move(value)); }
    static HttpHeader SecFetchDest(String &&value) { return HttpHeader(HttpHeaderNames::SecFetchDest, std::move(value)); }
    static HttpHeader SecPurpose(String &&value) { return HttpHeader(HttpHeaderNames::SecPurpose, std::move(value)); }
    static HttpHeader ServiceWorkerNavigationPreload(String &&value) { return HttpHeader(HttpHeaderNames::ServiceWorkerNavigationPreload, std::move(value)); }
    static HttpHeader ReportingEndpoints(String &&value) { return HttpHeader(HttpHeaderNames::ReportingEndpoints, std::move(value)); }
    static HttpHeader TransferEncoding(String &&value) { return HttpHeader(HttpHeaderNames::TransferEncoding, std::move(value)); }
    static HttpHeader Te(String &&value) { return HttpHeader(HttpHeaderNames::Te, std::move(value)); }
    static HttpHeader Trailer(String &&value) { return HttpHeader(HttpHeaderNames::Trailer, std::move(value)); }
    static HttpHeader SecWebSocketAccept(String &&value) { return HttpHeader(HttpHeaderNames::SecWebSocketAccept, std::move(value)); }
    static HttpHeader SecWebSocketExtensions(String &&value) { return HttpHeader(HttpHeaderNames::SecWebSocketExtensions, std::move(value)); }
    static HttpHeader SecWebSocketKey(String &&value) { return HttpHeader(HttpHeaderNames::SecWebSocketKey, std::move(value)); }
    static HttpHeader SecWebSocketProtocol(String &&value) { return HttpHeader(HttpHeaderNames::SecWebSocketProtocol, std::move(value)); }
    static HttpHeader SecWebSocketVersion(String &&value) { return HttpHeader(HttpHeaderNames::SecWebSocketVersion, std::move(value)); }
    static HttpHeader AltSvc(String &&value) { return HttpHeader(HttpHeaderNames::AltSvc, std::move(value)); }
    static HttpHeader AltUsed(String &&value) { return HttpHeader(HttpHeaderNames::AltUsed, std::move(value)); }
    static HttpHeader Date(String &&value) { return HttpHeader(HttpHeaderNames::Date, std::move(value)); }
    static HttpHeader Link(String &&value) { return HttpHeader(HttpHeaderNames::Link, std::move(value)); }
    static HttpHeader RetryAfter(String &&value) { return HttpHeader(HttpHeaderNames::RetryAfter, std::move(value)); }
    static HttpHeader ServerTiming(String &&value) { return HttpHeader(HttpHeaderNames::ServerTiming, std::move(value)); }
    static HttpHeader ServiceWorker(String &&value) { return HttpHeader(HttpHeaderNames::ServiceWorker, std::move(value)); }
    static HttpHeader ServiceWorkerAllowed(String &&value) { return HttpHeader(HttpHeaderNames::ServiceWorkerAllowed, std::move(value)); }
    static HttpHeader SourceMap(String &&value) { return HttpHeader(HttpHeaderNames::SourceMap, std::move(value)); }
    static HttpHeader Upgrade(String &&value) { return HttpHeader(HttpHeaderNames::Upgrade, std::move(value)); }
    static HttpHeader Priority(String &&value) { return HttpHeader(HttpHeaderNames::Priority, std::move(value)); }
    static HttpHeader AttributionReportingEligible(String &&value) { return HttpHeader(HttpHeaderNames::AttributionReportingEligible, std::move(value)); }
    static HttpHeader AttributionReportingRegisterSource(String &&value) { return HttpHeader(HttpHeaderNames::AttributionReportingRegisterSource, std::move(value)); }
    static HttpHeader AttributionReportingRegisterTrigger(String &&value) { return HttpHeader(HttpHeaderNames::AttributionReportingRegisterTrigger, std::move(value)); }
    static HttpHeader AcceptCh(String &&value) { return HttpHeader(HttpHeaderNames::AcceptCh, std::move(value)); }
    static HttpHeader DeviceMemory(String &&value) { return HttpHeader(HttpHeaderNames::DeviceMemory, std::move(value)); }
    static HttpHeader AcceptCharset(String &&value) { return HttpHeader(HttpHeaderNames::AcceptCharset, std::move(value)); }
    static HttpHeader Pragma(String &&value) { return HttpHeader(HttpHeaderNames::Pragma, std::move(value)); }
    static HttpHeader XForwardedFor(String &&value) { return HttpHeader(HttpHeaderNames::XForwardedFor, std::move(value)); }

    // Overloads for const String& (copy semantics)
    static HttpHeader WwwAuthenticate(const String &value) { return HttpHeader(HttpHeaderNames::WwwAuthenticate, value); }
    static HttpHeader Authorization(const String &value) { return HttpHeader(HttpHeaderNames::Authorization, value); }
    static HttpHeader ProxyAuthenticate(const String &value) { return HttpHeader(HttpHeaderNames::ProxyAuthenticate, value); }
    static HttpHeader ProxyAuthorization(const String &value) { return HttpHeader(HttpHeaderNames::ProxyAuthorization, value); }
    static HttpHeader Age(const String &value) { return HttpHeader(HttpHeaderNames::Age, value); }
    static HttpHeader CacheControl(const String &value) { return HttpHeader(HttpHeaderNames::CacheControl, value); }
    static HttpHeader ClearSiteData(const String &value) { return HttpHeader(HttpHeaderNames::ClearSiteData, value); }
    static HttpHeader Expires(const String &value) { return HttpHeader(HttpHeaderNames::Expires, value); }
    static HttpHeader LastModified(const String &value) { return HttpHeader(HttpHeaderNames::LastModified, value); }
    static HttpHeader ETag(const String &value) { return HttpHeader(HttpHeaderNames::ETag, value); }
    static HttpHeader IfMatch(const String &value) { return HttpHeader(HttpHeaderNames::IfMatch, value); }
    static HttpHeader IfNoneMatch(const String &value) { return HttpHeader(HttpHeaderNames::IfNoneMatch, value); }
    static HttpHeader IfModifiedSince(const String &value) { return HttpHeader(HttpHeaderNames::IfModifiedSince, value); }
    static HttpHeader IfUnmodifiedSince(const String &value) { return HttpHeader(HttpHeaderNames::IfUnmodifiedSince, value); }
    static HttpHeader Vary(const String &value) { return HttpHeader(HttpHeaderNames::Vary, value); }
    static HttpHeader Connection(const String &value) { return HttpHeader(HttpHeaderNames::Connection, value); }
    static HttpHeader KeepAlive(const String &value) { return HttpHeader(HttpHeaderNames::KeepAlive, value); }
    static HttpHeader Accept(const String &value) { return HttpHeader(HttpHeaderNames::Accept, value); }
    static HttpHeader AcceptEncoding(const String &value) { return HttpHeader(HttpHeaderNames::AcceptEncoding, value); }
    static HttpHeader AcceptLanguage(const String &value) { return HttpHeader(HttpHeaderNames::AcceptLanguage, value); }
    static HttpHeader AcceptPatch(const String &value) { return HttpHeader(HttpHeaderNames::AcceptPatch, value); }
    static HttpHeader AcceptPost(const String &value) { return HttpHeader(HttpHeaderNames::AcceptPost, value); }
    static HttpHeader Expect(const String &value) { return HttpHeader(HttpHeaderNames::Expect, value); }
    static HttpHeader MaxForwards(const String &value) { return HttpHeader(HttpHeaderNames::MaxForwards, value); }
    static HttpHeader Cookie(const String &value) { return HttpHeader(HttpHeaderNames::Cookie, value); }
    static HttpHeader SetCookie(const String &value) { return HttpHeader(HttpHeaderNames::SetCookie, value); }
    static HttpHeader AccessControlAllowCredentials(const String &value) { return HttpHeader(HttpHeaderNames::AccessControlAllowCredentials, value); }
    static HttpHeader AccessControlAllowHeaders(const String &value) { return HttpHeader(HttpHeaderNames::AccessControlAllowHeaders, value); }
    static HttpHeader AccessControlAllowMethods(const String &value) { return HttpHeader(HttpHeaderNames::AccessControlAllowMethods, value); }
    static HttpHeader AccessControlAllowOrigin(const String &value) { return HttpHeader(HttpHeaderNames::AccessControlAllowOrigin, value); }
    static HttpHeader AccessControlExposeHeaders(const String &value) { return HttpHeader(HttpHeaderNames::AccessControlExposeHeaders, value); }
    static HttpHeader AccessControlMaxAge(const String &value) { return HttpHeader(HttpHeaderNames::AccessControlMaxAge, value); }
    static HttpHeader AccessControlRequestHeaders(const String &value) { return HttpHeader(HttpHeaderNames::AccessControlRequestHeaders, value); }
    static HttpHeader AccessControlRequestMethod(const String &value) { return HttpHeader(HttpHeaderNames::AccessControlRequestMethod, value); }
    static HttpHeader Origin(const String &value) { return HttpHeader(HttpHeaderNames::Origin, value); }
    static HttpHeader TimingAllowOrigin(const String &value) { return HttpHeader(HttpHeaderNames::TimingAllowOrigin, value); }
    static HttpHeader ContentDisposition(const String &value) { return HttpHeader(HttpHeaderNames::ContentDisposition, value); }
    static HttpHeader IntegrityPolicy(const String &value) { return HttpHeader(HttpHeaderNames::IntegrityPolicy, value); }
    static HttpHeader IntegrityPolicyReportOnly(const String &value) { return HttpHeader(HttpHeaderNames::IntegrityPolicyReportOnly, value); }
    static HttpHeader ContentLength(const String &value) { return HttpHeader(HttpHeaderNames::ContentLength, value); }
    static HttpHeader ContentType(const String &value) { return HttpHeader(HttpHeaderNames::ContentType, value); }
    static HttpHeader ContentEncoding(const String &value) { return HttpHeader(HttpHeaderNames::ContentEncoding, value); }
    static HttpHeader ContentLanguage(const String &value) { return HttpHeader(HttpHeaderNames::ContentLanguage, value); }
    static HttpHeader ContentLocation(const String &value) { return HttpHeader(HttpHeaderNames::ContentLocation, value); }
    static HttpHeader Prefer(const String &value) { return HttpHeader(HttpHeaderNames::Prefer, value); }
    static HttpHeader PreferenceApplied(const String &value) { return HttpHeader(HttpHeaderNames::PreferenceApplied, value); }
    static HttpHeader Forwarded(const String &value) { return HttpHeader(HttpHeaderNames::Forwarded, value); }
    static HttpHeader Via(const String &value) { return HttpHeader(HttpHeaderNames::Via, value); }
    static HttpHeader AcceptRanges(const String &value) { return HttpHeader(HttpHeaderNames::AcceptRanges, value); }
    static HttpHeader Range(const String &value) { return HttpHeader(HttpHeaderNames::Range, value); }
    static HttpHeader IfRange(const String &value) { return HttpHeader(HttpHeaderNames::IfRange, value); }
    static HttpHeader ContentRange(const String &value) { return HttpHeader(HttpHeaderNames::ContentRange, value); }
    static HttpHeader Location(const String &value) { return HttpHeader(HttpHeaderNames::Location, value); }
    static HttpHeader Refresh(const String &value) { return HttpHeader(HttpHeaderNames::Refresh, value); }
    static HttpHeader From(const String &value) { return HttpHeader(HttpHeaderNames::From, value); }
    static HttpHeader Host(const String &value) { return HttpHeader(HttpHeaderNames::Host, value); }
    static HttpHeader Referer(const String &value) { return HttpHeader(HttpHeaderNames::Referer, value); }
    static HttpHeader ReferrerPolicy(const String &value) { return HttpHeader(HttpHeaderNames::ReferrerPolicy, value); }
    static HttpHeader UserAgent(const String &value) { return HttpHeader(HttpHeaderNames::UserAgent, value); }
    static HttpHeader Allow(const String &value) { return HttpHeader(HttpHeaderNames::Allow, value); }
    static HttpHeader Server(const String &value) { return HttpHeader(HttpHeaderNames::Server, value); }
    static HttpHeader CrossOriginEmbedderPolicy(const String &value) { return HttpHeader(HttpHeaderNames::CrossOriginEmbedderPolicy, value); }
    static HttpHeader CrossOriginOpenerPolicy(const String &value) { return HttpHeader(HttpHeaderNames::CROSS_ORIGIN_OPENER_POLICY, value); }
    static HttpHeader CrossOriginResourcePolicy(const String &value) { return HttpHeader(HttpHeaderNames::CROSS_ORIGIN_RESOURCE_POLICY, value); }
    static HttpHeader ContentSecurityPolicy(const String &value) { return HttpHeader(HttpHeaderNames::CONTENT_SECURITY_POLICY, value); }
    static HttpHeader ContentSecurityPolicyReportOnly(const String &value) { return HttpHeader(HttpHeaderNames::ContentSecurityPolicyReportOnly, value); }
    static HttpHeader PermissionsPolicy(const String &value) { return HttpHeader(HttpHeaderNames::PermissionsPolicy, value); }
    static HttpHeader StrictTransportSecurity(const String &value) { return HttpHeader(HttpHeaderNames::StrictTransportSecurity, value); }
    static HttpHeader UpgradeInsecureRequests(const String &value) { return HttpHeader(HttpHeaderNames::UpgradeInsecureRequests, value); }
    static HttpHeader XContentTypeOptions(const String &value) { return HttpHeader(HttpHeaderNames::XContentTypeOptions, value); }
    static HttpHeader XFrameOptions(const String &value) { return HttpHeader(HttpHeaderNames::XFrameOptions, value); }
    static HttpHeader XPermittedCrossDomainPolicies(const String &value) { return HttpHeader(HttpHeaderNames::XPermittedCrossDomainPolicies, value); }
    static HttpHeader XPoweredBy(const String &value) { return HttpHeader(HttpHeaderNames::XPoweredBy, value); }
    static HttpHeader XXssProtection(const String &value) { return HttpHeader(HttpHeaderNames::XXssProtection, value); }
    static HttpHeader SecFetchSite(const String &value) { return HttpHeader(HttpHeaderNames::SecFetchSite, value); }
    static HttpHeader SecFetchMode(const String &value) { return HttpHeader(HttpHeaderNames::SecFetchMode, value); }
    static HttpHeader SecFetchUser(const String &value) { return HttpHeader(HttpHeaderNames::SecFetchUser, value); }
    static HttpHeader SecFetchDest(const String &value) { return HttpHeader(HttpHeaderNames::SecFetchDest, value); }
    static HttpHeader SecPurpose(const String &value) { return HttpHeader(HttpHeaderNames::SecPurpose, value); }
    static HttpHeader ServiceWorkerNavigationPreload(const String &value) { return HttpHeader(HttpHeaderNames::ServiceWorkerNavigationPreload, value); }
    static HttpHeader ReportingEndpoints(const String &value) { return HttpHeader(HttpHeaderNames::ReportingEndpoints, value); }
    static HttpHeader TransferEncoding(const String &value) { return HttpHeader(HttpHeaderNames::TransferEncoding, value); }
    static HttpHeader Te(const String &value) { return HttpHeader(HttpHeaderNames::Te, value); }
    static HttpHeader Trailer(const String &value) { return HttpHeader(HttpHeaderNames::Trailer, value); }
    static HttpHeader SecWebSocketAccept(const String &value) { return HttpHeader(HttpHeaderNames::SecWebSocketAccept, value); }
    static HttpHeader SecWebSocketExtensions(const String &value) { return HttpHeader(HttpHeaderNames::SecWebSocketExtensions, value); }
    static HttpHeader SecWebSocketKey(const String &value) { return HttpHeader(HttpHeaderNames::SecWebSocketKey, value); }
    static HttpHeader SecWebSocketProtocol(const String &value) { return HttpHeader(HttpHeaderNames::SecWebSocketProtocol, value); }
    static HttpHeader SecWebSocketVersion(const String &value) { return HttpHeader(HttpHeaderNames::SecWebSocketVersion, value); }
    static HttpHeader AltSvc(const String &value) { return HttpHeader(HttpHeaderNames::AltSvc, value); }
    static HttpHeader AltUsed(const String &value) { return HttpHeader(HttpHeaderNames::AltUsed, value); }
    static HttpHeader Date(const String &value) { return HttpHeader(HttpHeaderNames::Date, value); }
    static HttpHeader Link(const String &value) { return HttpHeader(HttpHeaderNames::Link, value); }
    static HttpHeader RetryAfter(const String &value) { return HttpHeader(HttpHeaderNames::RetryAfter, value); }
    static HttpHeader ServerTiming(const String &value) { return HttpHeader(HttpHeaderNames::ServerTiming, value); }
    static HttpHeader ServiceWorker(const String &value) { return HttpHeader(HttpHeaderNames::ServiceWorker, value); }
    static HttpHeader ServiceWorkerAllowed(const String &value) { return HttpHeader(HttpHeaderNames::ServiceWorkerAllowed, value); }
    static HttpHeader SourceMap(const String &value) { return HttpHeader(HttpHeaderNames::SourceMap, value); }
    static HttpHeader Upgrade(const String &value) { return HttpHeader(HttpHeaderNames::Upgrade, value); }
    static HttpHeader Priority(const String &value) { return HttpHeader(HttpHeaderNames::Priority, value); }
    static HttpHeader AttributionReportingEligible(const String &value) { return HttpHeader(HttpHeaderNames::AttributionReportingEligible, value); }
    static HttpHeader AttributionReportingRegisterSource(const String &value) { return HttpHeader(HttpHeaderNames::AttributionReportingRegisterSource, value); }
    static HttpHeader AttributionReportingRegisterTrigger(const String &value) { return HttpHeader(HttpHeaderNames::AttributionReportingRegisterTrigger, value); }
    static HttpHeader AcceptCh(const String &value) { return HttpHeader(HttpHeaderNames::AcceptCh, value); }
    static HttpHeader DeviceMemory(const String &value) { return HttpHeader(HttpHeaderNames::DeviceMemory, value); }
    static HttpHeader AcceptCharset(const String &value) { return HttpHeader(HttpHeaderNames::AcceptCharset, value); }
    static HttpHeader Pragma(const String &value) { return HttpHeader(HttpHeaderNames::Pragma, value); }
    static HttpHeader XForwardedFor(const String &value) { return HttpHeader(HttpHeaderNames::XForwardedFor, value); }
  };

}
