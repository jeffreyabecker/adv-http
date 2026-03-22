#pragma once
// https://github.com/nodejs/llhttp
#include <Arduino.h>
#include "../llhttp/include/llhttp.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <algorithm>
#include <string>

namespace HttpServerAdvanced
{

  class HttpStatus
  {
  public:
    HttpStatus() : value_(0), description_() {}
    HttpStatus(uint16_t v) : value_(v), description_() {}

    HttpStatus(uint16_t v, const char *desc) : value_(v), description_(desc ? desc : "") {}
    HttpStatus(uint16_t v, std::string &&desc) : value_(v), description_(std::move(desc)) {}
    HttpStatus(uint16_t v, const std::string &desc = std::string()) : value_(v), description_(desc) {}

    // Copy constructor
    HttpStatus(const HttpStatus &other) = default;
    // Copy assignment
    HttpStatus &operator=(const HttpStatus &other) = default;
    // Move constructor
    HttpStatus(HttpStatus &&other) noexcept : value_(other.value_), description_(std::move(other.description_))
    {
      other.value_ = 0; // Reset the moved-from object
    }

    // Move assignment
    HttpStatus &operator=(HttpStatus &&other) noexcept
    {
      if (this != &other)
      {
        value_ = other.value_;
        description_ = std::move(other.description_);
        other.value_ = 0; // Reset the moved-from object
      }
      return *this;
    }

    // Implicit conversion from uint16_t
    operator uint16_t() const
    {
      return value_;
    }

    // Assignment from uint16_t
    HttpStatus &operator=(uint16_t v)
    {
      value_ = v;
      return *this;
    }

    // Equality operators
    // Comparison with another HttpStatus
    bool operator==(const HttpStatus &other) const
    {
      return value_ == other.value_;
    }
    bool operator!=(const HttpStatus &other) const
    {
      return value_ != other.value_;
    }
    bool operator<(const HttpStatus &other) const
    {
      return value_ < other.value_;
    }
    bool operator<=(const HttpStatus &other) const
    {
      return value_ <= other.value_;
    }
    bool operator>(const HttpStatus &other) const
    {
      return value_ > other.value_;
    }
    bool operator>=(const HttpStatus &other) const
    {
      return value_ >= other.value_;
    }

    // Comparison with uint16_t
    bool operator==(uint16_t v) const
    {
      return value_ == v;
    }
    bool operator!=(uint16_t v) const
    {
      return value_ != v;
    }
    bool operator<(uint16_t v) const
    {
      return value_ < v;
    }
    bool operator<=(uint16_t v) const
    {
      return value_ <= v;
    }
    bool operator>(uint16_t v) const
    {
      return value_ > v;
    }
    bool operator>=(uint16_t v) const
    {
      return value_ >= v;
    }

    uint16_t code() const
    {
      return value_;
    }

    const char* toString() const
    {
      if(description_.empty()){
        return DefaultDescription(value_);
      }
      return description_.c_str();
    }
    
    // Static methods for common HTTP status codes (PascalCase)
    static HttpStatus Continue(const std::string &desc = std::string()) { return HttpStatus(100, desc); }
    static HttpStatus SwitchingProtocols(const std::string &desc = std::string()) { return HttpStatus(101, desc); }
    static HttpStatus Processing(const std::string &desc = std::string()) { return HttpStatus(102, desc); }
    static HttpStatus Ok(const std::string &desc = std::string()) { return HttpStatus(200, desc); }
    static HttpStatus Created(const std::string &desc = std::string()) { return HttpStatus(201, desc); }
    static HttpStatus Accepted(const std::string &desc = std::string()) { return HttpStatus(202, desc); }
    static HttpStatus NonAuthoritativeInformation(const std::string &desc = std::string()) { return HttpStatus(203, desc); }
    static HttpStatus NoContent(const std::string &desc = std::string()) { return HttpStatus(204, desc); }
    static HttpStatus ResetContent(const std::string &desc = std::string()) { return HttpStatus(205, desc); }
    static HttpStatus PartialContent(const std::string &desc = std::string()) { return HttpStatus(206, desc); }
    static HttpStatus MultiStatus(const std::string &desc = std::string()) { return HttpStatus(207, desc); }
    static HttpStatus AlreadyReported(const std::string &desc = std::string()) { return HttpStatus(208, desc); }
    static HttpStatus ImUsed(const std::string &desc = std::string()) { return HttpStatus(226, desc); }
    static HttpStatus MultipleChoices(const std::string &desc = std::string()) { return HttpStatus(300, desc); }
    static HttpStatus MovedPermanently(const std::string &desc = std::string()) { return HttpStatus(301, desc); }
    static HttpStatus Found(const std::string &desc = std::string()) { return HttpStatus(302, desc); }
    static HttpStatus SeeOther(const std::string &desc = std::string()) { return HttpStatus(303, desc); }
    static HttpStatus NotModified(const std::string &desc = std::string()) { return HttpStatus(304, desc); }
    static HttpStatus UseProxy(const std::string &desc = std::string()) { return HttpStatus(305, desc); }
    static HttpStatus TemporaryRedirect(const std::string &desc = std::string()) { return HttpStatus(307, desc); }
    static HttpStatus PermanentRedirect(const std::string &desc = std::string()) { return HttpStatus(308, desc); }
    static HttpStatus BadRequest(const std::string &desc = std::string()) { return HttpStatus(400, desc); }
    static HttpStatus Unauthorized(const std::string &desc = std::string()) { return HttpStatus(401, desc); }
    static HttpStatus PaymentRequired(const std::string &desc = std::string()) { return HttpStatus(402, desc); }
    static HttpStatus Forbidden(const std::string &desc = std::string()) { return HttpStatus(403, desc); }
    static HttpStatus NotFound(const std::string &desc = std::string()) { return HttpStatus(404, desc); }
    static HttpStatus MethodNotAllowed(const std::string &desc = std::string()) { return HttpStatus(405, desc); }
    static HttpStatus NotAcceptable(const std::string &desc = std::string()) { return HttpStatus(406, desc); }
    static HttpStatus ProxyAuthenticationRequired(const std::string &desc = std::string()) { return HttpStatus(407, desc); }
    static HttpStatus RequestTimeout(const std::string &desc = std::string()) { return HttpStatus(408, desc); }
    static HttpStatus Conflict(const std::string &desc = std::string()) { return HttpStatus(409, desc); }
    static HttpStatus Gone(const std::string &desc = std::string()) { return HttpStatus(410, desc); }
    static HttpStatus LengthRequired(const std::string &desc = std::string()) { return HttpStatus(411, desc); }
    static HttpStatus PreconditionFailed(const std::string &desc = std::string()) { return HttpStatus(412, desc); }
    static HttpStatus PayloadTooLarge(const std::string &desc = std::string()) { return HttpStatus(413, desc); }
    static HttpStatus UriTooLong(const std::string &desc = std::string()) { return HttpStatus(414, desc); }
    static HttpStatus UnsupportedMediaType(const std::string &desc = std::string()) { return HttpStatus(415, desc); }
    static HttpStatus RangeNotSatisfiable(const std::string &desc = std::string()) { return HttpStatus(416, desc); }
    static HttpStatus ExpectationFailed(const std::string &desc = std::string()) { return HttpStatus(417, desc); }
    static HttpStatus ImATeapot(const std::string &desc = std::string()) { return HttpStatus(418, desc); }
    static HttpStatus MisdirectedRequest(const std::string &desc = std::string()) { return HttpStatus(421, desc); }
    static HttpStatus UnprocessableEntity(const std::string &desc = std::string()) { return HttpStatus(422, desc); }
    static HttpStatus Locked(const std::string &desc = std::string()) { return HttpStatus(423, desc); }
    static HttpStatus FailedDependency(const std::string &desc = std::string()) { return HttpStatus(424, desc); }
    static HttpStatus TooEarly(const std::string &desc = std::string()) { return HttpStatus(425, desc); }
    static HttpStatus UpgradeRequired(const std::string &desc = std::string()) { return HttpStatus(426, desc); }
    static HttpStatus PreconditionRequired(const std::string &desc = std::string()) { return HttpStatus(428, desc); }
    static HttpStatus TooManyRequests(const std::string &desc = std::string()) { return HttpStatus(429, desc); }
    static HttpStatus RequestHeaderFieldsTooLarge(const std::string &desc = std::string()) { return HttpStatus(431, desc); }
    static HttpStatus UnavailableForLegalReasons(const std::string &desc = std::string()) { return HttpStatus(451, desc); }
    static HttpStatus InternalServerError(const std::string &desc = std::string()) { return HttpStatus(500, desc); }
    static HttpStatus NotImplemented(const std::string &desc = std::string()) { return HttpStatus(501, desc); }
    static HttpStatus BadGateway(const std::string &desc = std::string()) { return HttpStatus(502, desc); }
    static HttpStatus ServiceUnavailable(const std::string &desc = std::string()) { return HttpStatus(503, desc); }
    static HttpStatus GatewayTimeout(const std::string &desc = std::string()) { return HttpStatus(504, desc); }
    static HttpStatus HttpVersionNotSupported(const std::string &desc = std::string()) { return HttpStatus(505, desc); }
    static HttpStatus VariantAlsoNegotiates(const std::string &desc = std::string()) { return HttpStatus(506, desc); }
    static HttpStatus InsufficientStorage(const std::string &desc = std::string()) { return HttpStatus(507, desc); }
    static HttpStatus LoopDetected(const std::string &desc = std::string()) { return HttpStatus(508, desc); }
    static HttpStatus NotExtended(const std::string &desc = std::string()) { return HttpStatus(510, desc); }
    static HttpStatus NetworkAuthenticationRequired(const std::string &desc = std::string()) { return HttpStatus(511, desc); }

  private:
    static const char *DefaultDescription(uint16_t code)
    {
      switch (code)
      {
      case 100: return "Continue";
      case 101: return "Switching Protocols";
      case 102: return "Processing";
      case 103: return "Early Hints";
      case 200: return "OK";
      case 201: return "Created";
      case 202: return "Accepted";
      case 203: return "Non-Authoritative Information";
      case 204: return "No Content";
      case 205: return "Reset Content";
      case 206: return "Partial Content";
      case 207: return "Multi-Status";
      case 208: return "Already Reported";
      case 226: return "IM Used";
      case 300: return "Multiple Choices";
      case 301: return "Moved Permanently";
      case 302: return "Found";
      case 303: return "See Other";
      case 304: return "Not Modified";
      case 305: return "Use Proxy";
      case 307: return "Temporary Redirect";
      case 308: return "Permanent Redirect";
      case 400: return "Bad Request";
      case 401: return "Unauthorized";
      case 402: return "Payment Required";
      case 403: return "Forbidden";
      case 404: return "Not Found";
      case 405: return "Method Not Allowed";
      case 406: return "Not Acceptable";
      case 407: return "Proxy Authentication Required";
      case 408: return "Request Timeout";
      case 409: return "Conflict";
      case 410: return "Gone";
      case 411: return "Length Required";
      case 412: return "Precondition Failed";
      case 413: return "Payload Too Large";
      case 414: return "URI Too Long";
      case 415: return "Unsupported Media Type";
      case 416: return "Range Not Satisfiable";
      case 417: return "Expectation Failed";
      case 418: return "I'm a teapot";
      case 421: return "Misdirected Request";
      case 422: return "Unprocessable Entity";
      case 423: return "Locked";
      case 424: return "Failed Dependency";
      case 425: return "Too Early";
      case 426: return "Upgrade Required";
      case 428: return "Precondition Required";
      case 429: return "Too Many Requests";
      case 431: return "Request Header Fields Too Large";
      case 451: return "Unavailable For Legal Reasons";
      case 500: return "Internal Server Error";
      case 501: return "Not Implemented";
      case 502: return "Bad Gateway";
      case 503: return "Service Unavailable";
      case 504: return "Gateway Timeout";
      case 505: return "HTTP Version Not Supported";
      case 506: return "Variant Also Negotiates";
      case 507: return "Insufficient Storage";
      case 508: return "Loop Detected";
      case 510: return "Not Extended";
      case 511: return "Network Authentication Required";
      default: return "Unknown";
      }
    }

    uint16_t value_;
    std::string description_;
  };

} // namespace HttpServerAdvanced
