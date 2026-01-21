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

  class HttpStatus
  {
  public:
    HttpStatus() : value_(0), description_() {} 
    HttpStatus(uint16_t v) : value_(v), description_() {}
    HttpStatus(enum http_status s) : value_(static_cast<uint16_t>(s)), description_() {}
    HttpStatus(uint16_t v, const char *desc) : value_(v), description_(desc) {}
    HttpStatus(uint16_t v, arduino::String &&desc) : value_(v), description_(std::move(desc)) {}
    HttpStatus(uint16_t v, const arduino::String &desc = "") : value_(v), description_(desc) {}

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

    // Comparison with enum http_status
    bool operator==(enum http_status s) const
    {
      return value_ == static_cast<uint16_t>(s);
    }
    bool operator!=(enum http_status s) const
    {
      return value_ != static_cast<uint16_t>(s);
    }
    bool operator<(enum http_status s) const
    {
      return value_ < static_cast<uint16_t>(s);
    }
    bool operator<=(enum http_status s) const
    {
      return value_ <= static_cast<uint16_t>(s);
    }
    bool operator>(enum http_status s) const
    {
      return value_ > static_cast<uint16_t>(s);
    }
    bool operator>=(enum http_status s) const
    {
      return value_ >= static_cast<uint16_t>(s);
    }

    uint16_t code() const
    {
      return value_;
    }

    const char* toString() const
    {
      if(description_.isEmpty()){
        return http_status_str(static_cast<enum http_status>(value_));
      }
      return description_.c_str();
    }
    
    // Static methods for common HTTP status codes (PascalCase)
    static HttpStatus Continue(const arduino::String &desc = "") { return HttpStatus(100, desc); }
    static HttpStatus SwitchingProtocols(const arduino::String &desc = "") { return HttpStatus(101, desc); }
    static HttpStatus Processing(const arduino::String &desc = "") { return HttpStatus(102, desc); }
    static HttpStatus Ok(const arduino::String &desc = "") { return HttpStatus(200, desc); }
    static HttpStatus Created(const arduino::String &desc = "") { return HttpStatus(201, desc); }
    static HttpStatus Accepted(const arduino::String &desc = "") { return HttpStatus(202, desc); }
    static HttpStatus NonAuthoritativeInformation(const arduino::String &desc = "") { return HttpStatus(203, desc); }
    static HttpStatus NoContent(const arduino::String &desc = "") { return HttpStatus(204, desc); }
    static HttpStatus ResetContent(const arduino::String &desc = "") { return HttpStatus(205, desc); }
    static HttpStatus PartialContent(const arduino::String &desc = "") { return HttpStatus(206, desc); }
    static HttpStatus MultiStatus(const arduino::String &desc = "") { return HttpStatus(207, desc); }
    static HttpStatus AlreadyReported(const arduino::String &desc = "") { return HttpStatus(208, desc); }
    static HttpStatus ImUsed(const arduino::String &desc = "") { return HttpStatus(226, desc); }
    static HttpStatus MultipleChoices(const arduino::String &desc = "") { return HttpStatus(300, desc); }
    static HttpStatus MovedPermanently(const arduino::String &desc = "") { return HttpStatus(301, desc); }
    static HttpStatus Found(const arduino::String &desc = "") { return HttpStatus(302, desc); }
    static HttpStatus SeeOther(const arduino::String &desc = "") { return HttpStatus(303, desc); }
    static HttpStatus NotModified(const arduino::String &desc = "") { return HttpStatus(304, desc); }
    static HttpStatus UseProxy(const arduino::String &desc = "") { return HttpStatus(305, desc); }
    static HttpStatus TemporaryRedirect(const arduino::String &desc = "") { return HttpStatus(307, desc); }
    static HttpStatus PermanentRedirect(const arduino::String &desc = "") { return HttpStatus(308, desc); }
    static HttpStatus BadRequest(const arduino::String &desc = "") { return HttpStatus(400, desc); }
    static HttpStatus Unauthorized(const arduino::String &desc = "") { return HttpStatus(401, desc); }
    static HttpStatus PaymentRequired(const arduino::String &desc = "") { return HttpStatus(402, desc); }
    static HttpStatus Forbidden(const arduino::String &desc = "") { return HttpStatus(403, desc); }
    static HttpStatus NotFound(const arduino::String &desc = "") { return HttpStatus(404, desc); }
    static HttpStatus MethodNotAllowed(const arduino::String &desc = "") { return HttpStatus(405, desc); }
    static HttpStatus NotAcceptable(const arduino::String &desc = "") { return HttpStatus(406, desc); }
    static HttpStatus ProxyAuthenticationRequired(const arduino::String &desc = "") { return HttpStatus(407, desc); }
    static HttpStatus RequestTimeout(const arduino::String &desc = "") { return HttpStatus(408, desc); }
    static HttpStatus Conflict(const arduino::String &desc = "") { return HttpStatus(409, desc); }
    static HttpStatus Gone(const arduino::String &desc = "") { return HttpStatus(410, desc); }
    static HttpStatus LengthRequired(const arduino::String &desc = "") { return HttpStatus(411, desc); }
    static HttpStatus PreconditionFailed(const arduino::String &desc = "") { return HttpStatus(412, desc); }
    static HttpStatus PayloadTooLarge(const arduino::String &desc = "") { return HttpStatus(413, desc); }
    static HttpStatus UriTooLong(const arduino::String &desc = "") { return HttpStatus(414, desc); }
    static HttpStatus UnsupportedMediaType(const arduino::String &desc = "") { return HttpStatus(415, desc); }
    static HttpStatus RangeNotSatisfiable(const arduino::String &desc = "") { return HttpStatus(416, desc); }
    static HttpStatus ExpectationFailed(const arduino::String &desc = "") { return HttpStatus(417, desc); }
    static HttpStatus ImATeapot(const arduino::String &desc = "") { return HttpStatus(418, desc); }
    static HttpStatus MisdirectedRequest(const arduino::String &desc = "") { return HttpStatus(421, desc); }
    static HttpStatus UnprocessableEntity(const arduino::String &desc = "") { return HttpStatus(422, desc); }
    static HttpStatus Locked(const arduino::String &desc = "") { return HttpStatus(423, desc); }
    static HttpStatus FailedDependency(const arduino::String &desc = "") { return HttpStatus(424, desc); }
    static HttpStatus TooEarly(const arduino::String &desc = "") { return HttpStatus(425, desc); }
    static HttpStatus UpgradeRequired(const arduino::String &desc = "") { return HttpStatus(426, desc); }
    static HttpStatus PreconditionRequired(const arduino::String &desc = "") { return HttpStatus(428, desc); }
    static HttpStatus TooManyRequests(const arduino::String &desc = "") { return HttpStatus(429, desc); }
    static HttpStatus RequestHeaderFieldsTooLarge(const arduino::String &desc = "") { return HttpStatus(431, desc); }
    static HttpStatus UnavailableForLegalReasons(const arduino::String &desc = "") { return HttpStatus(451, desc); }
    static HttpStatus InternalServerError(const arduino::String &desc = "") { return HttpStatus(500, desc); }
    static HttpStatus NotImplemented(const arduino::String &desc = "") { return HttpStatus(501, desc); }
    static HttpStatus BadGateway(const arduino::String &desc = "") { return HttpStatus(502, desc); }
    static HttpStatus ServiceUnavailable(const arduino::String &desc = "") { return HttpStatus(503, desc); }
    static HttpStatus GatewayTimeout(const arduino::String &desc = "") { return HttpStatus(504, desc); }
    static HttpStatus HttpVersionNotSupported(const arduino::String &desc = "") { return HttpStatus(505, desc); }
    static HttpStatus VariantAlsoNegotiates(const arduino::String &desc = "") { return HttpStatus(506, desc); }
    static HttpStatus InsufficientStorage(const arduino::String &desc = "") { return HttpStatus(507, desc); }
    static HttpStatus LoopDetected(const arduino::String &desc = "") { return HttpStatus(508, desc); }
    static HttpStatus NotExtended(const arduino::String &desc = "") { return HttpStatus(510, desc); }
    static HttpStatus NetworkAuthenticationRequired(const arduino::String &desc = "") { return HttpStatus(511, desc); }

  private:
    uint16_t value_;
    arduino::String description_;
  };

} // namespace HttpServerAdvanced
