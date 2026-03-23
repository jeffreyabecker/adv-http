#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <algorithm>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

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
    std::string name_;
    std::string value_;

  public:
    HttpHeader() = default;
    explicit HttpHeader(std::string name, std::string value) : name_(std::move(name)), value_(std::move(value)) {}
    explicit HttpHeader(std::string_view name, std::string_view value) : name_(name), value_(value) {}
    explicit HttpHeader(std::string_view name, std::string value) : name_(name), value_(std::move(value)) {}
    explicit HttpHeader(const char *name, const char *value) : name_(name != nullptr ? name : ""), value_(value != nullptr ? value : "") {}

    ~HttpHeader() = default;
    HttpHeader(const HttpHeader &) = default;
    HttpHeader(HttpHeader &&) noexcept = default;
    HttpHeader &operator=(const HttpHeader &) = default;
    HttpHeader &operator=(HttpHeader &&) noexcept = default;

    std::string_view name() const { return nameView(); }

    std::string_view nameView() const
    {
      return std::string_view(name_.data(), name_.size());
    }

    std::string_view value() const { return valueView(); }

    std::string_view valueView() const
    {
      return std::string_view(value_.data(), value_.size());
    }

#if __cpp_structured_bindings
    // Enable structured bindings (C++17 and later)
    friend auto tie(const HttpHeader &header)
    {
      return std::tie(header.name_, header.value_);
    }
#endif

    static std::vector<std::pair<std::string, std::optional<std::string>>> parseDirectives(std::string_view val = "");
    static HttpHeader WwwAuthenticate(std::string_view value) { return HttpHeader(HttpHeaderNames::WwwAuthenticate, value); }
    static HttpHeader Authorization(std::string_view value) { return HttpHeader(HttpHeaderNames::Authorization, value); }
    static HttpHeader ProxyAuthenticate(std::string_view value) { return HttpHeader(HttpHeaderNames::ProxyAuthenticate, value); }
    static HttpHeader ProxyAuthorization(std::string_view value) { return HttpHeader(HttpHeaderNames::ProxyAuthorization, value); }
    static HttpHeader Age(std::string_view value) { return HttpHeader(HttpHeaderNames::Age, value); }
    static HttpHeader CacheControl(std::string_view value) { return HttpHeader(HttpHeaderNames::CacheControl, value); }
    static HttpHeader ClearSiteData(std::string_view value) { return HttpHeader(HttpHeaderNames::ClearSiteData, value); }
    static HttpHeader Expires(std::string_view value) { return HttpHeader(HttpHeaderNames::Expires, value); }
    static HttpHeader LastModified(std::string_view value) { return HttpHeader(HttpHeaderNames::LastModified, value); }
    static HttpHeader ETag(std::string_view value) { return HttpHeader(HttpHeaderNames::ETag, value); }
    static HttpHeader IfMatch(std::string_view value) { return HttpHeader(HttpHeaderNames::IfMatch, value); }
    static HttpHeader IfNoneMatch(std::string_view value) { return HttpHeader(HttpHeaderNames::IfNoneMatch, value); }
    static HttpHeader IfModifiedSince(std::string_view value) { return HttpHeader(HttpHeaderNames::IfModifiedSince, value); }
    static HttpHeader IfUnmodifiedSince(std::string_view value) { return HttpHeader(HttpHeaderNames::IfUnmodifiedSince, value); }
    static HttpHeader Vary(std::string_view value) { return HttpHeader(HttpHeaderNames::Vary, value); }
    static HttpHeader Connection(std::string_view value) { return HttpHeader(HttpHeaderNames::Connection, value); }
    static HttpHeader KeepAlive(std::string_view value) { return HttpHeader(HttpHeaderNames::KeepAlive, value); }
    static HttpHeader Accept(std::string_view value) { return HttpHeader(HttpHeaderNames::Accept, value); }
    static HttpHeader AcceptEncoding(std::string_view value) { return HttpHeader(HttpHeaderNames::AcceptEncoding, value); }
    static HttpHeader AcceptLanguage(std::string_view value) { return HttpHeader(HttpHeaderNames::AcceptLanguage, value); }
    static HttpHeader AcceptPatch(std::string_view value) { return HttpHeader(HttpHeaderNames::AcceptPatch, value); }
    static HttpHeader AcceptPost(std::string_view value) { return HttpHeader(HttpHeaderNames::AcceptPost, value); }
    static HttpHeader Expect(std::string_view value) { return HttpHeader(HttpHeaderNames::Expect, value); }
    static HttpHeader MaxForwards(std::string_view value) { return HttpHeader(HttpHeaderNames::MaxForwards, value); }
    static HttpHeader Cookie(std::string_view value) { return HttpHeader(HttpHeaderNames::Cookie, value); }
    static HttpHeader SetCookie(std::string_view value) { return HttpHeader(HttpHeaderNames::SetCookie, value); }
    static HttpHeader AccessControlAllowCredentials(std::string_view value) { return HttpHeader(HttpHeaderNames::AccessControlAllowCredentials, value); }
    static HttpHeader AccessControlAllowHeaders(std::string_view value) { return HttpHeader(HttpHeaderNames::AccessControlAllowHeaders, value); }
    static HttpHeader AccessControlAllowMethods(std::string_view value) { return HttpHeader(HttpHeaderNames::AccessControlAllowMethods, value); }
    static HttpHeader AccessControlAllowOrigin(std::string_view value) { return HttpHeader(HttpHeaderNames::AccessControlAllowOrigin, value); }
    static HttpHeader AccessControlExposeHeaders(std::string_view value) { return HttpHeader(HttpHeaderNames::AccessControlExposeHeaders, value); }
    static HttpHeader AccessControlMaxAge(std::string_view value) { return HttpHeader(HttpHeaderNames::AccessControlMaxAge, value); }
    static HttpHeader AccessControlRequestHeaders(std::string_view value) { return HttpHeader(HttpHeaderNames::AccessControlRequestHeaders, value); }
    static HttpHeader AccessControlRequestMethod(std::string_view value) { return HttpHeader(HttpHeaderNames::AccessControlRequestMethod, value); }
    static HttpHeader Origin(std::string_view value) { return HttpHeader(HttpHeaderNames::Origin, value); }
    static HttpHeader TimingAllowOrigin(std::string_view value) { return HttpHeader(HttpHeaderNames::TimingAllowOrigin, value); }
    static HttpHeader ContentDisposition(std::string_view value) { return HttpHeader(HttpHeaderNames::ContentDisposition, value); }
    static HttpHeader IntegrityPolicy(std::string_view value) { return HttpHeader(HttpHeaderNames::IntegrityPolicy, value); }
    static HttpHeader IntegrityPolicyReportOnly(std::string_view value) { return HttpHeader(HttpHeaderNames::IntegrityPolicyReportOnly, value); }
    static HttpHeader ContentLength(std::string_view value) { return HttpHeader(HttpHeaderNames::ContentLength, value); }
    static HttpHeader ContentType(std::string_view value) { return HttpHeader(HttpHeaderNames::ContentType, value); }
    static HttpHeader ContentEncoding(std::string_view value) { return HttpHeader(HttpHeaderNames::ContentEncoding, value); }
    static HttpHeader ContentLanguage(std::string_view value) { return HttpHeader(HttpHeaderNames::ContentLanguage, value); }
    static HttpHeader ContentLocation(std::string_view value) { return HttpHeader(HttpHeaderNames::ContentLocation, value); }
    static HttpHeader Prefer(std::string_view value) { return HttpHeader(HttpHeaderNames::Prefer, value); }
    static HttpHeader PreferenceApplied(std::string_view value) { return HttpHeader(HttpHeaderNames::PreferenceApplied, value); }
    static HttpHeader Forwarded(std::string_view value) { return HttpHeader(HttpHeaderNames::Forwarded, value); }
    static HttpHeader Via(std::string_view value) { return HttpHeader(HttpHeaderNames::Via, value); }
    static HttpHeader AcceptRanges(std::string_view value) { return HttpHeader(HttpHeaderNames::AcceptRanges, value); }
    static HttpHeader Range(std::string_view value) { return HttpHeader(HttpHeaderNames::Range, value); }
    static HttpHeader IfRange(std::string_view value) { return HttpHeader(HttpHeaderNames::IfRange, value); }
    static HttpHeader ContentRange(std::string_view value) { return HttpHeader(HttpHeaderNames::ContentRange, value); }
    static HttpHeader Location(std::string_view value) { return HttpHeader(HttpHeaderNames::Location, value); }
    static HttpHeader Refresh(std::string_view value) { return HttpHeader(HttpHeaderNames::Refresh, value); }
    static HttpHeader From(std::string_view value) { return HttpHeader(HttpHeaderNames::From, value); }
    static HttpHeader Host(std::string_view value) { return HttpHeader(HttpHeaderNames::Host, value); }
    static HttpHeader Referer(std::string_view value) { return HttpHeader(HttpHeaderNames::Referer, value); }
    static HttpHeader ReferrerPolicy(std::string_view value) { return HttpHeader(HttpHeaderNames::ReferrerPolicy, value); }
    static HttpHeader UserAgent(std::string_view value) { return HttpHeader(HttpHeaderNames::UserAgent, value); }
    static HttpHeader Allow(std::string_view value) { return HttpHeader(HttpHeaderNames::Allow, value); }
    static HttpHeader Server(std::string_view value) { return HttpHeader(HttpHeaderNames::Server, value); }
    static HttpHeader CrossOriginEmbedderPolicy(std::string_view value) { return HttpHeader(HttpHeaderNames::CrossOriginEmbedderPolicy, value); }
    static HttpHeader CrossOriginOpenerPolicy(std::string_view value) { return HttpHeader(HttpHeaderNames::CROSS_ORIGIN_OPENER_POLICY, value); }
    static HttpHeader CrossOriginResourcePolicy(std::string_view value) { return HttpHeader(HttpHeaderNames::CROSS_ORIGIN_RESOURCE_POLICY, value); }
    static HttpHeader ContentSecurityPolicy(std::string_view value) { return HttpHeader(HttpHeaderNames::CONTENT_SECURITY_POLICY, value); }
    static HttpHeader ContentSecurityPolicyReportOnly(std::string_view value) { return HttpHeader(HttpHeaderNames::ContentSecurityPolicyReportOnly, value); }
    static HttpHeader PermissionsPolicy(std::string_view value) { return HttpHeader(HttpHeaderNames::PermissionsPolicy, value); }
    static HttpHeader StrictTransportSecurity(std::string_view value) { return HttpHeader(HttpHeaderNames::StrictTransportSecurity, value); }
    static HttpHeader UpgradeInsecureRequests(std::string_view value) { return HttpHeader(HttpHeaderNames::UpgradeInsecureRequests, value); }
    static HttpHeader XContentTypeOptions(std::string_view value) { return HttpHeader(HttpHeaderNames::XContentTypeOptions, value); }
    static HttpHeader XFrameOptions(std::string_view value) { return HttpHeader(HttpHeaderNames::XFrameOptions, value); }
    static HttpHeader XPermittedCrossDomainPolicies(std::string_view value) { return HttpHeader(HttpHeaderNames::XPermittedCrossDomainPolicies, value); }
    static HttpHeader XPoweredBy(std::string_view value) { return HttpHeader(HttpHeaderNames::XPoweredBy, value); }
    static HttpHeader XXssProtection(std::string_view value) { return HttpHeader(HttpHeaderNames::XXssProtection, value); }
    static HttpHeader SecFetchSite(std::string_view value) { return HttpHeader(HttpHeaderNames::SecFetchSite, value); }
    static HttpHeader SecFetchMode(std::string_view value) { return HttpHeader(HttpHeaderNames::SecFetchMode, value); }
    static HttpHeader SecFetchUser(std::string_view value) { return HttpHeader(HttpHeaderNames::SecFetchUser, value); }
    static HttpHeader SecFetchDest(std::string_view value) { return HttpHeader(HttpHeaderNames::SecFetchDest, value); }
    static HttpHeader SecPurpose(std::string_view value) { return HttpHeader(HttpHeaderNames::SecPurpose, value); }
    static HttpHeader ServiceWorkerNavigationPreload(std::string_view value) { return HttpHeader(HttpHeaderNames::ServiceWorkerNavigationPreload, value); }
    static HttpHeader ReportingEndpoints(std::string_view value) { return HttpHeader(HttpHeaderNames::ReportingEndpoints, value); }
    static HttpHeader TransferEncoding(std::string_view value) { return HttpHeader(HttpHeaderNames::TransferEncoding, value); }
    static HttpHeader Te(std::string_view value) { return HttpHeader(HttpHeaderNames::Te, value); }
    static HttpHeader Trailer(std::string_view value) { return HttpHeader(HttpHeaderNames::Trailer, value); }
    static HttpHeader SecWebSocketAccept(std::string_view value) { return HttpHeader(HttpHeaderNames::SecWebSocketAccept, value); }
    static HttpHeader SecWebSocketExtensions(std::string_view value) { return HttpHeader(HttpHeaderNames::SecWebSocketExtensions, value); }
    static HttpHeader SecWebSocketKey(std::string_view value) { return HttpHeader(HttpHeaderNames::SecWebSocketKey, value); }
    static HttpHeader SecWebSocketProtocol(std::string_view value) { return HttpHeader(HttpHeaderNames::SecWebSocketProtocol, value); }
    static HttpHeader SecWebSocketVersion(std::string_view value) { return HttpHeader(HttpHeaderNames::SecWebSocketVersion, value); }
    static HttpHeader AltSvc(std::string_view value) { return HttpHeader(HttpHeaderNames::AltSvc, value); }
    static HttpHeader AltUsed(std::string_view value) { return HttpHeader(HttpHeaderNames::AltUsed, value); }
    static HttpHeader Date(std::string_view value) { return HttpHeader(HttpHeaderNames::Date, value); }
    static HttpHeader Link(std::string_view value) { return HttpHeader(HttpHeaderNames::Link, value); }
    static HttpHeader RetryAfter(std::string_view value) { return HttpHeader(HttpHeaderNames::RetryAfter, value); }
    static HttpHeader ServerTiming(std::string_view value) { return HttpHeader(HttpHeaderNames::ServerTiming, value); }
    static HttpHeader ServiceWorker(std::string_view value) { return HttpHeader(HttpHeaderNames::ServiceWorker, value); }
    static HttpHeader ServiceWorkerAllowed(std::string_view value) { return HttpHeader(HttpHeaderNames::ServiceWorkerAllowed, value); }
    static HttpHeader SourceMap(std::string_view value) { return HttpHeader(HttpHeaderNames::SourceMap, value); }
    static HttpHeader Upgrade(std::string_view value) { return HttpHeader(HttpHeaderNames::Upgrade, value); }
    static HttpHeader Priority(std::string_view value) { return HttpHeader(HttpHeaderNames::Priority, value); }
    static HttpHeader AttributionReportingEligible(std::string_view value) { return HttpHeader(HttpHeaderNames::AttributionReportingEligible, value); }
    static HttpHeader AttributionReportingRegisterSource(std::string_view value) { return HttpHeader(HttpHeaderNames::AttributionReportingRegisterSource, value); }
    static HttpHeader AttributionReportingRegisterTrigger(std::string_view value) { return HttpHeader(HttpHeaderNames::AttributionReportingRegisterTrigger, value); }
    static HttpHeader AcceptCh(std::string_view value) { return HttpHeader(HttpHeaderNames::AcceptCh, value); }
    static HttpHeader DeviceMemory(std::string_view value) { return HttpHeader(HttpHeaderNames::DeviceMemory, value); }
    static HttpHeader AcceptCharset(std::string_view value) { return HttpHeader(HttpHeaderNames::AcceptCharset, value); }
    static HttpHeader Pragma(std::string_view value) { return HttpHeader(HttpHeaderNames::Pragma, value); }
    static HttpHeader XForwardedFor(std::string_view value) { return HttpHeader(HttpHeaderNames::XForwardedFor, value); }
  };

}
