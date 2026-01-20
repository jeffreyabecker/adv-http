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
  };

  class HttpHeaders
  {
  public:
    static constexpr const char *WWW_AUTHENTICATE = "WWW-Authenticate";
    static constexpr const char *AUTHORIZATION = "Authorization";
    static constexpr const char *PROXY_AUTHENTICATE = "Proxy-Authenticate";
    static constexpr const char *PROXY_AUTHORIZATION = "Proxy-Authorization";
    static constexpr const char *AGE = "Age";
    static constexpr const char *CACHE_CONTROL = "Cache-Control";
    static constexpr const char *CLEAR_SITE_DATA = "Clear-Site-Data";
    static constexpr const char *EXPIRES = "Expires";
    static constexpr const char *LAST_MODIFIED = "Last-Modified";
    static constexpr const char *ETAG = "ETag";
    static constexpr const char *IF_MATCH = "If-Match";
    static constexpr const char *IF_NONE_MATCH = "If-None-Match";
    static constexpr const char *IF_MODIFIED_SINCE = "If-Modified-Since";
    static constexpr const char *IF_UNMODIFIED_SINCE = "If-Unmodified-Since";
    static constexpr const char *VARY = "Vary";
    static constexpr const char *CONNECTION = "Connection";
    static constexpr const char *KEEP_ALIVE = "Keep-Alive";
    static constexpr const char *ACCEPT = "Accept";
    static constexpr const char *ACCEPT_ENCODING = "Accept-Encoding";
    static constexpr const char *ACCEPT_LANGUAGE = "Accept-Language";
    static constexpr const char *ACCEPT_PATCH = "Accept-Patch";
    static constexpr const char *ACCEPT_POST = "Accept-Post";
    static constexpr const char *EXPECT = "Expect";
    static constexpr const char *MAX_FORWARDS = "Max-Forwards";
    static constexpr const char *COOKIE = "Cookie";
    static constexpr const char *SET_COOKIE = "Set-Cookie";
    static constexpr const char *ACCESS_CONTROL_ALLOW_CREDENTIALS = "Access-Control-Allow-Credentials";
    static constexpr const char *ACCESS_CONTROL_ALLOW_HEADERS = "Access-Control-Allow-Headers";
    static constexpr const char *ACCESS_CONTROL_ALLOW_METHODS = "Access-Control-Allow-Methods";
    static constexpr const char *ACCESS_CONTROL_ALLOW_ORIGIN = "Access-Control-Allow-Origin";
    static constexpr const char *ACCESS_CONTROL_EXPOSE_HEADERS = "Access-Control-Expose-Headers";
    static constexpr const char *ACCESS_CONTROL_MAX_AGE = "Access-Control-Max-Age";
    static constexpr const char *ACCESS_CONTROL_REQUEST_HEADERS = "Access-Control-Request-Headers";
    static constexpr const char *ACCESS_CONTROL_REQUEST_METHOD = "Access-Control-Request-Method";
    static constexpr const char *ORIGIN = "Origin";
    static constexpr const char *TIMING_ALLOW_ORIGIN = "Timing-Allow-Origin";
    static constexpr const char *CONTENT_DISPOSITION = "Content-Disposition";
    static constexpr const char *INTEGRITY_POLICY = "Integrity-Policy";
    static constexpr const char *INTEGRITY_POLICY_REPORT_ONLY = "Integrity-Policy-Report-Only";
    static constexpr const char *CONTENT_LENGTH = "Content-Length";
    static constexpr const char *CONTENT_TYPE = "Content-Type";
    static constexpr const char *CONTENT_ENCODING = "Content-Encoding";
    static constexpr const char *CONTENT_LANGUAGE = "Content-Language";
    static constexpr const char *CONTENT_LOCATION = "Content-Location";
    static constexpr const char *PREFER = "Prefer";
    static constexpr const char *PREFERENCE_APPLIED = "Preference-Applied";
    static constexpr const char *FORWARDED = "Forwarded";
    static constexpr const char *VIA = "Via";
    static constexpr const char *ACCEPT_RANGES = "Accept-Ranges";
    static constexpr const char *RANGE = "Range";
    static constexpr const char *IF_RANGE = "If-Range";
    static constexpr const char *CONTENT_RANGE = "Content-Range";
    static constexpr const char *LOCATION = "Location";
    static constexpr const char *REFRESH = "Refresh";
    static constexpr const char *FROM = "From";
    static constexpr const char *HOST = "Host";
    static constexpr const char *REFERER = "Referer";
    static constexpr const char *REFERRER_POLICY = "Referrer-Policy";
    static constexpr const char *USER_AGENT = "User-Agent";
    static constexpr const char *ALLOW = "Allow";
    static constexpr const char *SERVER = "Server";
    static constexpr const char *CROSS_ORIGIN_EMBEDDER_POLICY = "Cross-Origin-Embedder-Policy";
    static constexpr const char *CROSS_ORIGIN_OPENER_POLICY = "Cross-Origin-Opener-Policy";
    static constexpr const char *CROSS_ORIGIN_RESOURCE_POLICY = "Cross-Origin-Resource-Policy";
    static constexpr const char *CONTENT_SECURITY_POLICY = "Content-Security-Policy";
    static constexpr const char *CONTENT_SECURITY_POLICY_REPORT_ONLY = "Content-Security-Policy-Report-Only";
    static constexpr const char *PERMISSIONS_POLICY = "Permissions-Policy";
    static constexpr const char *STRICT_TRANSPORT_SECURITY = "Strict-Transport-Security";
    static constexpr const char *UPGRADE_INSECURE_REQUESTS = "Upgrade-Insecure-Requests";
    static constexpr const char *X_CONTENT_TYPE_OPTIONS = "X-Content-Type-Options";
    static constexpr const char *X_FRAME_OPTIONS = "X-Frame-Options";
    static constexpr const char *X_PERMITTED_CROSS_DOMAIN_POLICIES = "X-Permitted-Cross-Domain-Policies";
    static constexpr const char *X_POWERED_BY = "X-Powered-By";
    static constexpr const char *X_XSS_PROTECTION = "X-XSS-Protection";
    static constexpr const char *SEC_FETCH_SITE = "Sec-Fetch-Site";
    static constexpr const char *SEC_FETCH_MODE = "Sec-Fetch-Mode";
    static constexpr const char *SEC_FETCH_USER = "Sec-Fetch-User";
    static constexpr const char *SEC_FETCH_DEST = "Sec-Fetch-Dest";
    static constexpr const char *SEC_PURPOSE = "Sec-Purpose";
    static constexpr const char *SERVICE_WORKER_NAVIGATION_PRELOAD = "Service-Worker-Navigation-Preload";
    static constexpr const char *REPORTING_ENDPOINTS = "Reporting-Endpoints";
    static constexpr const char *TRANSFER_ENCODING = "Transfer-Encoding";
    static constexpr const char *TE = "TE";
    static constexpr const char *TRAILER = "Trailer";
    static constexpr const char *SEC_WEBSOCKET_ACCEPT = "Sec-WebSocket-Accept";
    static constexpr const char *SEC_WEBSOCKET_EXTENSIONS = "Sec-WebSocket-Extensions";
    static constexpr const char *SEC_WEBSOCKET_KEY = "Sec-WebSocket-Key";
    static constexpr const char *SEC_WEBSOCKET_PROTOCOL = "Sec-WebSocket-Protocol";
    static constexpr const char *SEC_WEBSOCKET_VERSION = "Sec-WebSocket-Version";
    static constexpr const char *ALT_SVC = "Alt-Svc";
    static constexpr const char *ALT_USED = "Alt-Used";
    static constexpr const char *DATE = "Date";
    static constexpr const char *LINK = "Link";
    static constexpr const char *RETRY_AFTER = "Retry-After";
    static constexpr const char *SERVER_TIMING = "Server-Timing";
    static constexpr const char *SERVICE_WORKER = "Service-Worker";
    static constexpr const char *SERVICE_WORKER_ALLOWED = "Service-Worker-Allowed";
    static constexpr const char *SOURCEMAP = "SourceMap";
    static constexpr const char *UPGRADE = "Upgrade";
    static constexpr const char *PRIORITY = "Priority";
    static constexpr const char *ATTRIBUTION_REPORTING_ELIGIBLE = "Attribution-Reporting-Eligible";
    static constexpr const char *ATTRIBUTION_REPORTING_REGISTER_SOURCE = "Attribution-Reporting-Register-Source";
    static constexpr const char *ATTRIBUTION_REPORTING_REGISTER_TRIGGER = "Attribution-Reporting-Register-Trigger";
    static constexpr const char *ACCEPT_CH = "Accept-CH";
    static constexpr const char *DEVICE_MEMORY = "Device-Memory";
    static constexpr const char *ACCEPT_CHARSET = "Accept-Charset";
    static constexpr const char *PRAGMA = "Pragma";
    static constexpr const char *X_FORWARDED_FOR = "X-Forwarded-For";

    // Factory methods
    static HttpHeader WwwAuthenticate(const String &value) { return HttpHeader(WWW_AUTHENTICATE, value); }
    static HttpHeader WwwAuthenticate(const char *value) { return HttpHeader(WWW_AUTHENTICATE, value); }
    static HttpHeader Authorization(const String &value) { return HttpHeader(AUTHORIZATION, value); }
    static HttpHeader Authorization(const char *value) { return HttpHeader(AUTHORIZATION, value); }
    static HttpHeader ProxyAuthenticate(const String &value) { return HttpHeader(PROXY_AUTHENTICATE, value); }
    static HttpHeader ProxyAuthenticate(const char *value) { return HttpHeader(PROXY_AUTHENTICATE, value); }
    static HttpHeader ProxyAuthorization(const String &value) { return HttpHeader(PROXY_AUTHORIZATION, value); }
    static HttpHeader ProxyAuthorization(const char *value) { return HttpHeader(PROXY_AUTHORIZATION, value); }
    static HttpHeader Age(const String &value) { return HttpHeader(AGE, value); }
    static HttpHeader Age(const char *value) { return HttpHeader(AGE, value); }
    static HttpHeader CacheControl(const String &value) { return HttpHeader(CACHE_CONTROL, value); }
    static HttpHeader CacheControl(const char *value) { return HttpHeader(CACHE_CONTROL, value); }
    static HttpHeader ClearSiteData(const String &value) { return HttpHeader(CLEAR_SITE_DATA, value); }
    static HttpHeader ClearSiteData(const char *value) { return HttpHeader(CLEAR_SITE_DATA, value); }
    static HttpHeader Expires(const String &value) { return HttpHeader(EXPIRES, value); }
    static HttpHeader Expires(const char *value) { return HttpHeader(EXPIRES, value); }
    static HttpHeader LastModified(const String &value) { return HttpHeader(LAST_MODIFIED, value); }
    static HttpHeader LastModified(const char *value) { return HttpHeader(LAST_MODIFIED, value); }
    static HttpHeader ETag(const String &value) { return HttpHeader(ETAG, value); }
    static HttpHeader ETag(const char *value) { return HttpHeader(ETAG, value); }
    static HttpHeader IfMatch(const String &value) { return HttpHeader(IF_MATCH, value); }
    static HttpHeader IfMatch(const char *value) { return HttpHeader(IF_MATCH, value); }
    static HttpHeader IfNoneMatch(const String &value) { return HttpHeader(IF_NONE_MATCH, value); }
    static HttpHeader IfNoneMatch(const char *value) { return HttpHeader(IF_NONE_MATCH, value); }
    static HttpHeader IfModifiedSince(const String &value) { return HttpHeader(IF_MODIFIED_SINCE, value); }
    static HttpHeader IfModifiedSince(const char *value) { return HttpHeader(IF_MODIFIED_SINCE, value); }
    static HttpHeader IfUnmodifiedSince(const String &value) { return HttpHeader(IF_UNMODIFIED_SINCE, value); }
    static HttpHeader IfUnmodifiedSince(const char *value) { return HttpHeader(IF_UNMODIFIED_SINCE, value); }
    static HttpHeader Vary(const String &value) { return HttpHeader(VARY, value); }
    static HttpHeader Vary(const char *value) { return HttpHeader(VARY, value); }
    static HttpHeader Connection(const String &value) { return HttpHeader(CONNECTION, value); }
    static HttpHeader Connection(const char *value) { return HttpHeader(CONNECTION, value); }
    static HttpHeader KeepAlive(const String &value) { return HttpHeader(KEEP_ALIVE, value); }
    static HttpHeader KeepAlive(const char *value) { return HttpHeader(KEEP_ALIVE, value); }
    static HttpHeader Accept(const String &value) { return HttpHeader(ACCEPT, value); }
    static HttpHeader Accept(const char *value) { return HttpHeader(ACCEPT, value); }
    static HttpHeader AcceptEncoding(const String &value) { return HttpHeader(ACCEPT_ENCODING, value); }
    static HttpHeader AcceptEncoding(const char *value) { return HttpHeader(ACCEPT_ENCODING, value); }
    static HttpHeader AcceptLanguage(const String &value) { return HttpHeader(ACCEPT_LANGUAGE, value); }
    static HttpHeader AcceptLanguage(const char *value) { return HttpHeader(ACCEPT_LANGUAGE, value); }
    static HttpHeader AcceptPatch(const String &value) { return HttpHeader(ACCEPT_PATCH, value); }
    static HttpHeader AcceptPatch(const char *value) { return HttpHeader(ACCEPT_PATCH, value); }
    static HttpHeader AcceptPost(const String &value) { return HttpHeader(ACCEPT_POST, value); }
    static HttpHeader AcceptPost(const char *value) { return HttpHeader(ACCEPT_POST, value); }
    static HttpHeader Expect(const String &value) { return HttpHeader(EXPECT, value); }
    static HttpHeader Expect(const char *value) { return HttpHeader(EXPECT, value); }
    static HttpHeader MaxForwards(const String &value) { return HttpHeader(MAX_FORWARDS, value); }
    static HttpHeader MaxForwards(const char *value) { return HttpHeader(MAX_FORWARDS, value); }
    static HttpHeader Cookie(const String &value) { return HttpHeader(COOKIE, value); }
    static HttpHeader Cookie(const char *value) { return HttpHeader(COOKIE, value); }
    static HttpHeader SetCookie(const String &value) { return HttpHeader(SET_COOKIE, value); }
    static HttpHeader SetCookie(const char *value) { return HttpHeader(SET_COOKIE, value); }
    static HttpHeader AccessControlAllowCredentials(const String &value) { return HttpHeader(ACCESS_CONTROL_ALLOW_CREDENTIALS, value); }
    static HttpHeader AccessControlAllowCredentials(const char *value) { return HttpHeader(ACCESS_CONTROL_ALLOW_CREDENTIALS, value); }
    static HttpHeader AccessControlAllowHeaders(const String &value) { return HttpHeader(ACCESS_CONTROL_ALLOW_HEADERS, value); }
    static HttpHeader AccessControlAllowHeaders(const char *value) { return HttpHeader(ACCESS_CONTROL_ALLOW_HEADERS, value); }
    static HttpHeader AccessControlAllowMethods(const String &value) { return HttpHeader(ACCESS_CONTROL_ALLOW_METHODS, value); }
    static HttpHeader AccessControlAllowMethods(const char *value) { return HttpHeader(ACCESS_CONTROL_ALLOW_METHODS, value); }
    static HttpHeader AccessControlAllowOrigin(const String &value) { return HttpHeader(ACCESS_CONTROL_ALLOW_ORIGIN, value); }
    static HttpHeader AccessControlAllowOrigin(const char *value) { return HttpHeader(ACCESS_CONTROL_ALLOW_ORIGIN, value); }
    static HttpHeader AccessControlExposeHeaders(const String &value) { return HttpHeader(ACCESS_CONTROL_EXPOSE_HEADERS, value); }
    static HttpHeader AccessControlExposeHeaders(const char *value) { return HttpHeader(ACCESS_CONTROL_EXPOSE_HEADERS, value); }
    static HttpHeader AccessControlMaxAge(const String &value) { return HttpHeader(ACCESS_CONTROL_MAX_AGE, value); }
    static HttpHeader AccessControlMaxAge(const char *value) { return HttpHeader(ACCESS_CONTROL_MAX_AGE, value); }
    static HttpHeader AccessControlRequestHeaders(const String &value) { return HttpHeader(ACCESS_CONTROL_REQUEST_HEADERS, value); }
    static HttpHeader AccessControlRequestHeaders(const char *value) { return HttpHeader(ACCESS_CONTROL_REQUEST_HEADERS, value); }
    static HttpHeader AccessControlRequestMethod(const String &value) { return HttpHeader(ACCESS_CONTROL_REQUEST_METHOD, value); }
    static HttpHeader AccessControlRequestMethod(const char *value) { return HttpHeader(ACCESS_CONTROL_REQUEST_METHOD, value); }
    static HttpHeader Origin(const String &value) { return HttpHeader(ORIGIN, value); }
    static HttpHeader Origin(const char *value) { return HttpHeader(ORIGIN, value); }
    static HttpHeader TimingAllowOrigin(const String &value) { return HttpHeader(TIMING_ALLOW_ORIGIN, value); }
    static HttpHeader TimingAllowOrigin(const char *value) { return HttpHeader(TIMING_ALLOW_ORIGIN, value); }
    static HttpHeader ContentDisposition(const String &value) { return HttpHeader(CONTENT_DISPOSITION, value); }
    static HttpHeader ContentDisposition(const char *value) { return HttpHeader(CONTENT_DISPOSITION, value); }
    static HttpHeader IntegrityPolicy(const String &value) { return HttpHeader(INTEGRITY_POLICY, value); }
    static HttpHeader IntegrityPolicy(const char *value) { return HttpHeader(INTEGRITY_POLICY, value); }
    static HttpHeader IntegrityPolicyReportOnly(const String &value) { return HttpHeader(INTEGRITY_POLICY_REPORT_ONLY, value); }
    static HttpHeader IntegrityPolicyReportOnly(const char *value) { return HttpHeader(INTEGRITY_POLICY_REPORT_ONLY, value); }
    static HttpHeader ContentLength(const String &value) { return HttpHeader(CONTENT_LENGTH, value); }
    static HttpHeader ContentLength(const char *value) { return HttpHeader(CONTENT_LENGTH, value); }
    static HttpHeader ContentLength(int value) { return HttpHeader(CONTENT_LENGTH, String(value)); }
    static HttpHeader ContentType(const String &value) { return HttpHeader(CONTENT_TYPE, value); }
    static HttpHeader ContentType(const char *value) { return HttpHeader(CONTENT_TYPE, value); }
    static HttpHeader ContentEncoding(const String &value) { return HttpHeader(CONTENT_ENCODING, value); }
    static HttpHeader ContentEncoding(const char *value) { return HttpHeader(CONTENT_ENCODING, value); }
    static HttpHeader ContentLanguage(const String &value) { return HttpHeader(CONTENT_LANGUAGE, value); }
    static HttpHeader ContentLanguage(const char *value) { return HttpHeader(CONTENT_LANGUAGE, value); }
    static HttpHeader ContentLocation(const String &value) { return HttpHeader(CONTENT_LOCATION, value); }
    static HttpHeader ContentLocation(const char *value) { return HttpHeader(CONTENT_LOCATION, value); }
    static HttpHeader Prefer(const String &value) { return HttpHeader(PREFER, value); }
    static HttpHeader Prefer(const char *value) { return HttpHeader(PREFER, value); }
    static HttpHeader PreferenceApplied(const String &value) { return HttpHeader(PREFERENCE_APPLIED, value); }
    static HttpHeader PreferenceApplied(const char *value) { return HttpHeader(PREFERENCE_APPLIED, value); }
    static HttpHeader Forwarded(const String &value) { return HttpHeader(FORWARDED, value); }
    static HttpHeader Forwarded(const char *value) { return HttpHeader(FORWARDED, value); }
    static HttpHeader Via(const String &value) { return HttpHeader(VIA, value); }
    static HttpHeader Via(const char *value) { return HttpHeader(VIA, value); }
    static HttpHeader AcceptRanges(const String &value) { return HttpHeader(ACCEPT_RANGES, value); }
    static HttpHeader AcceptRanges(const char *value) { return HttpHeader(ACCEPT_RANGES, value); }
    static HttpHeader Range(const String &value) { return HttpHeader(RANGE, value); }
    static HttpHeader Range(const char *value) { return HttpHeader(RANGE, value); }
    static HttpHeader IfRange(const String &value) { return HttpHeader(IF_RANGE, value); }
    static HttpHeader IfRange(const char *value) { return HttpHeader(IF_RANGE, value); }
    static HttpHeader ContentRange(const String &value) { return HttpHeader(CONTENT_RANGE, value); }
    static HttpHeader ContentRange(const char *value) { return HttpHeader(CONTENT_RANGE, value); }
    static HttpHeader Location(const String &value) { return HttpHeader(LOCATION, value); }
    static HttpHeader Location(const char *value) { return HttpHeader(LOCATION, value); }
    static HttpHeader Refresh(const String &value) { return HttpHeader(REFRESH, value); }
    static HttpHeader Refresh(const char *value) { return HttpHeader(REFRESH, value); }
    static HttpHeader From(const String &value) { return HttpHeader(FROM, value); }
    static HttpHeader From(const char *value) { return HttpHeader(FROM, value); }
    static HttpHeader Host(const String &value) { return HttpHeader(HOST, value); }
    static HttpHeader Host(const char *value) { return HttpHeader(HOST, value); }
    static HttpHeader Referer(const String &value) { return HttpHeader(REFERER, value); }
    static HttpHeader Referer(const char *value) { return HttpHeader(REFERER, value); }
    static HttpHeader ReferrerPolicy(const String &value) { return HttpHeader(REFERRER_POLICY, value); }
    static HttpHeader ReferrerPolicy(const char *value) { return HttpHeader(REFERRER_POLICY, value); }
    static HttpHeader UserAgent(const String &value) { return HttpHeader(USER_AGENT, value); }
    static HttpHeader UserAgent(const char *value) { return HttpHeader(USER_AGENT, value); }
    static HttpHeader Allow(const String &value) { return HttpHeader(ALLOW, value); }
    static HttpHeader Allow(const char *value) { return HttpHeader(ALLOW, value); }
    static HttpHeader Server(const String &value) { return HttpHeader(SERVER, value); }
    static HttpHeader Server(const char *value) { return HttpHeader(SERVER, value); }
    static HttpHeader CrossOriginEmbedderPolicy(const String &value) { return HttpHeader(CROSS_ORIGIN_EMBEDDER_POLICY, value); }
    static HttpHeader CrossOriginEmbedderPolicy(const char *value) { return HttpHeader(CROSS_ORIGIN_EMBEDDER_POLICY, value); }
    static HttpHeader CrossOriginOpenerPolicy(const String &value) { return HttpHeader(CROSS_ORIGIN_OPENER_POLICY, value); }
    static HttpHeader CrossOriginOpenerPolicy(const char *value) { return HttpHeader(CROSS_ORIGIN_OPENER_POLICY, value); }
    static HttpHeader CrossOriginResourcePolicy(const String &value) { return HttpHeader(CROSS_ORIGIN_RESOURCE_POLICY, value); }
    static HttpHeader CrossOriginResourcePolicy(const char *value) { return HttpHeader(CROSS_ORIGIN_RESOURCE_POLICY, value); }
    static HttpHeader ContentSecurityPolicy(const String &value) { return HttpHeader(CONTENT_SECURITY_POLICY, value); }
    static HttpHeader ContentSecurityPolicy(const char *value) { return HttpHeader(CONTENT_SECURITY_POLICY, value); }
    static HttpHeader ContentSecurityPolicyReportOnly(const String &value) { return HttpHeader(CONTENT_SECURITY_POLICY_REPORT_ONLY, value); }
    static HttpHeader ContentSecurityPolicyReportOnly(const char *value) { return HttpHeader(CONTENT_SECURITY_POLICY_REPORT_ONLY, value); }
    static HttpHeader PermissionsPolicy(const String &value) { return HttpHeader(PERMISSIONS_POLICY, value); }
    static HttpHeader PermissionsPolicy(const char *value) { return HttpHeader(PERMISSIONS_POLICY, value); }
    static HttpHeader StrictTransportSecurity(const String &value) { return HttpHeader(STRICT_TRANSPORT_SECURITY, value); }
    static HttpHeader StrictTransportSecurity(const char *value) { return HttpHeader(STRICT_TRANSPORT_SECURITY, value); }
    static HttpHeader UpgradeInsecureRequests(const String &value) { return HttpHeader(UPGRADE_INSECURE_REQUESTS, value); }
    static HttpHeader UpgradeInsecureRequests(const char *value) { return HttpHeader(UPGRADE_INSECURE_REQUESTS, value); }
    static HttpHeader XContentTypeOptions(const String &value) { return HttpHeader(X_CONTENT_TYPE_OPTIONS, value); }
    static HttpHeader XContentTypeOptions(const char *value) { return HttpHeader(X_CONTENT_TYPE_OPTIONS, value); }
    static HttpHeader XFrameOptions(const String &value) { return HttpHeader(X_FRAME_OPTIONS, value); }
    static HttpHeader XFrameOptions(const char *value) { return HttpHeader(X_FRAME_OPTIONS, value); }
    static HttpHeader XPermittedCrossDomainPolicies(const String &value) { return HttpHeader(X_PERMITTED_CROSS_DOMAIN_POLICIES, value); }
    static HttpHeader XPermittedCrossDomainPolicies(const char *value) { return HttpHeader(X_PERMITTED_CROSS_DOMAIN_POLICIES, value); }
    static HttpHeader XPoweredBy(const String &value) { return HttpHeader(X_POWERED_BY, value); }
    static HttpHeader XPoweredBy(const char *value) { return HttpHeader(X_POWERED_BY, value); }
    static HttpHeader XXssProtection(const String &value) { return HttpHeader(X_XSS_PROTECTION, value); }
    static HttpHeader XXssProtection(const char *value) { return HttpHeader(X_XSS_PROTECTION, value); }
    static HttpHeader SecFetchSite(const String &value) { return HttpHeader(SEC_FETCH_SITE, value); }
    static HttpHeader SecFetchSite(const char *value) { return HttpHeader(SEC_FETCH_SITE, value); }
    static HttpHeader SecFetchMode(const String &value) { return HttpHeader(SEC_FETCH_MODE, value); }
    static HttpHeader SecFetchMode(const char *value) { return HttpHeader(SEC_FETCH_MODE, value); }
    static HttpHeader SecFetchUser(const String &value) { return HttpHeader(SEC_FETCH_USER, value); }
    static HttpHeader SecFetchUser(const char *value) { return HttpHeader(SEC_FETCH_USER, value); }
    static HttpHeader SecFetchDest(const String &value) { return HttpHeader(SEC_FETCH_DEST, value); }
    static HttpHeader SecFetchDest(const char *value) { return HttpHeader(SEC_FETCH_DEST, value); }
    static HttpHeader SecPurpose(const String &value) { return HttpHeader(SEC_PURPOSE, value); }
    static HttpHeader SecPurpose(const char *value) { return HttpHeader(SEC_PURPOSE, value); }
    static HttpHeader ServiceWorkerNavigationPreload(const String &value) { return HttpHeader(SERVICE_WORKER_NAVIGATION_PRELOAD, value); }
    static HttpHeader ServiceWorkerNavigationPreload(const char *value) { return HttpHeader(SERVICE_WORKER_NAVIGATION_PRELOAD, value); }
    static HttpHeader ReportingEndpoints(const String &value) { return HttpHeader(REPORTING_ENDPOINTS, value); }
    static HttpHeader ReportingEndpoints(const char *value) { return HttpHeader(REPORTING_ENDPOINTS, value); }
    static HttpHeader TransferEncoding(const String &value) { return HttpHeader(TRANSFER_ENCODING, value); }
    static HttpHeader TransferEncoding(const char *value) { return HttpHeader(TRANSFER_ENCODING, value); }
    static HttpHeader Te(const String &value) { return HttpHeader(TE, value); }
    static HttpHeader Te(const char *value) { return HttpHeader(TE, value); }
    static HttpHeader Trailer(const String &value) { return HttpHeader(TRAILER, value); }
    static HttpHeader Trailer(const char *value) { return HttpHeader(TRAILER, value); }
    static HttpHeader SecWebSocketAccept(const String &value) { return HttpHeader(SEC_WEBSOCKET_ACCEPT, value); }
    static HttpHeader SecWebSocketAccept(const char *value) { return HttpHeader(SEC_WEBSOCKET_ACCEPT, value); }
    static HttpHeader SecWebSocketExtensions(const String &value) { return HttpHeader(SEC_WEBSOCKET_EXTENSIONS, value); }
    static HttpHeader SecWebSocketExtensions(const char *value) { return HttpHeader(SEC_WEBSOCKET_EXTENSIONS, value); }
    static HttpHeader SecWebSocketKey(const String &value) { return HttpHeader(SEC_WEBSOCKET_KEY, value); }
    static HttpHeader SecWebSocketKey(const char *value) { return HttpHeader(SEC_WEBSOCKET_KEY, value); }
    static HttpHeader SecWebSocketProtocol(const String &value) { return HttpHeader(SEC_WEBSOCKET_PROTOCOL, value); }
    static HttpHeader SecWebSocketProtocol(const char *value) { return HttpHeader(SEC_WEBSOCKET_PROTOCOL, value); }
    static HttpHeader SecWebSocketVersion(const String &value) { return HttpHeader(SEC_WEBSOCKET_VERSION, value); }
    static HttpHeader SecWebSocketVersion(const char *value) { return HttpHeader(SEC_WEBSOCKET_VERSION, value); }
    static HttpHeader AltSvc(const String &value) { return HttpHeader(ALT_SVC, value); }
    static HttpHeader AltSvc(const char *value) { return HttpHeader(ALT_SVC, value); }
    static HttpHeader AltUsed(const String &value) { return HttpHeader(ALT_USED, value); }
    static HttpHeader AltUsed(const char *value) { return HttpHeader(ALT_USED, value); }
    static HttpHeader Date(const String &value) { return HttpHeader(DATE, value); }
    static HttpHeader Date(const char *value) { return HttpHeader(DATE, value); }
    static HttpHeader Link(const String &value) { return HttpHeader(LINK, value); }
    static HttpHeader Link(const char *value) { return HttpHeader(LINK, value); }
    static HttpHeader RetryAfter(const String &value) { return HttpHeader(RETRY_AFTER, value); }
    static HttpHeader RetryAfter(const char *value) { return HttpHeader(RETRY_AFTER, value); }
    static HttpHeader ServerTiming(const String &value) { return HttpHeader(SERVER_TIMING, value); }
    static HttpHeader ServerTiming(const char *value) { return HttpHeader(SERVER_TIMING, value); }
    static HttpHeader ServiceWorker(const String &value) { return HttpHeader(SERVICE_WORKER, value); }
    static HttpHeader ServiceWorker(const char *value) { return HttpHeader(SERVICE_WORKER, value); }
    static HttpHeader ServiceWorkerAllowed(const String &value) { return HttpHeader(SERVICE_WORKER_ALLOWED, value); }
    static HttpHeader ServiceWorkerAllowed(const char *value) { return HttpHeader(SERVICE_WORKER_ALLOWED, value); }
    static HttpHeader SourceMap(const String &value) { return HttpHeader(SOURCEMAP, value); }
    static HttpHeader SourceMap(const char *value) { return HttpHeader(SOURCEMAP, value); }
    static HttpHeader Upgrade(const String &value) { return HttpHeader(UPGRADE, value); }
    static HttpHeader Upgrade(const char *value) { return HttpHeader(UPGRADE, value); }
    static HttpHeader Priority(const String &value) { return HttpHeader(PRIORITY, value); }
    static HttpHeader Priority(const char *value) { return HttpHeader(PRIORITY, value); }
    static HttpHeader AttributionReportingEligible(const String &value) { return HttpHeader(ATTRIBUTION_REPORTING_ELIGIBLE, value); }
    static HttpHeader AttributionReportingEligible(const char *value) { return HttpHeader(ATTRIBUTION_REPORTING_ELIGIBLE, value); }
    static HttpHeader AttributionReportingRegisterSource(const String &value) { return HttpHeader(ATTRIBUTION_REPORTING_REGISTER_SOURCE, value); }
    static HttpHeader AttributionReportingRegisterSource(const char *value) { return HttpHeader(ATTRIBUTION_REPORTING_REGISTER_SOURCE, value); }
    static HttpHeader AttributionReportingRegisterTrigger(const String &value) { return HttpHeader(ATTRIBUTION_REPORTING_REGISTER_TRIGGER, value); }
    static HttpHeader AttributionReportingRegisterTrigger(const char *value) { return HttpHeader(ATTRIBUTION_REPORTING_REGISTER_TRIGGER, value); }
    static HttpHeader AcceptCh(const String &value) { return HttpHeader(ACCEPT_CH, value); }
    static HttpHeader AcceptCh(const char *value) { return HttpHeader(ACCEPT_CH, value); }
    static HttpHeader DeviceMemory(const String &value) { return HttpHeader(DEVICE_MEMORY, value); }
    static HttpHeader DeviceMemory(const char *value) { return HttpHeader(DEVICE_MEMORY, value); }
    static HttpHeader AcceptCharset(const String &value) { return HttpHeader(ACCEPT_CHARSET, value); }
    static HttpHeader AcceptCharset(const char *value) { return HttpHeader(ACCEPT_CHARSET, value); }
    static HttpHeader Pragma(const String &value) { return HttpHeader(PRAGMA, value); }
    static HttpHeader Pragma(const char *value) { return HttpHeader(PRAGMA, value); }
    static HttpHeader XForwardedFor(const String &value) { return HttpHeader(X_FORWARDED_FOR, value); }
    static HttpHeader XForwardedFor(const char *value) { return HttpHeader(X_FORWARDED_FOR, value); }
  };

  class HttpHeadersCollection : public std::vector<HttpHeader>
  {

  public:
    HttpHeadersCollection() = default;
    HttpHeadersCollection(std::initializer_list<HttpHeader> headers);
    HttpHeadersCollection(std::initializer_list<std::pair<const String &, const String &>> headers);
    std::optional<HttpHeader> find(const String &name) const;
    bool exists(const String &name) const;
    bool exists(const String& name, const String &value) const;

    void set(const HttpHeader &header, bool forceOverwrite = false);
    void set(HttpHeader &&header, bool forceOverwrite = false);
    void set(const String &name, const String &value, bool forceOverwrite = false);
    void remove(const String &name);
    void remove(const String& name, const String &value);
  };



}