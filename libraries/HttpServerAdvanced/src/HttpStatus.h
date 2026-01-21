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
    static inline HttpStatus Continue(const arduino::String &desc = "") { return HttpStatus(100, desc); }
    static inline HttpStatus SwitchingProtocols(const arduino::String &desc = "") { return HttpStatus(101, desc); }
    static inline HttpStatus Processing(const arduino::String &desc = "") { return HttpStatus(102, desc); }
    static inline HttpStatus Ok(const arduino::String &desc = "") { return HttpStatus(200, desc); }
    static inline HttpStatus Created(const arduino::String &desc = "") { return HttpStatus(201, desc); }
    static inline HttpStatus Accepted(const arduino::String &desc = "") { return HttpStatus(202, desc); }
    static inline HttpStatus NonAuthoritativeInformation(const arduino::String &desc = "") { return HttpStatus(203, desc); }
    static inline HttpStatus NoContent(const arduino::String &desc = "") { return HttpStatus(204, desc); }
    static inline HttpStatus ResetContent(const arduino::String &desc = "") { return HttpStatus(205, desc); }
    static inline HttpStatus PartialContent(const arduino::String &desc = "") { return HttpStatus(206, desc); }
    static inline HttpStatus MultiStatus(const arduino::String &desc = "") { return HttpStatus(207, desc); }
    static inline HttpStatus AlreadyReported(const arduino::String &desc = "") { return HttpStatus(208, desc); }
    static inline HttpStatus ImUsed(const arduino::String &desc = "") { return HttpStatus(226, desc); }
    static inline HttpStatus MultipleChoices(const arduino::String &desc = "") { return HttpStatus(300, desc); }
    static inline HttpStatus MovedPermanently(const arduino::String &desc = "") { return HttpStatus(301, desc); }
    static inline HttpStatus Found(const arduino::String &desc = "") { return HttpStatus(302, desc); }
    static inline HttpStatus SeeOther(const arduino::String &desc = "") { return HttpStatus(303, desc); }
    static inline HttpStatus NotModified(const arduino::String &desc = "") { return HttpStatus(304, desc); }
    static inline HttpStatus UseProxy(const arduino::String &desc = "") { return HttpStatus(305, desc); }
    static inline HttpStatus TemporaryRedirect(const arduino::String &desc = "") { return HttpStatus(307, desc); }
    static inline HttpStatus PermanentRedirect(const arduino::String &desc = "") { return HttpStatus(308, desc); }
    static inline HttpStatus BadRequest(const arduino::String &desc = "") { return HttpStatus(400, desc); }
    static inline HttpStatus Unauthorized(const arduino::String &desc = "") { return HttpStatus(401, desc); }
    static inline HttpStatus PaymentRequired(const arduino::String &desc = "") { return HttpStatus(402, desc); }
    static inline HttpStatus Forbidden(const arduino::String &desc = "") { return HttpStatus(403, desc); }
    static inline HttpStatus NotFound(const arduino::String &desc = "") { return HttpStatus(404, desc); }
    static inline HttpStatus MethodNotAllowed(const arduino::String &desc = "") { return HttpStatus(405, desc); }
    static inline HttpStatus NotAcceptable(const arduino::String &desc = "") { return HttpStatus(406, desc); }
    static inline HttpStatus ProxyAuthenticationRequired(const arduino::String &desc = "") { return HttpStatus(407, desc); }
    static inline HttpStatus RequestTimeout(const arduino::String &desc = "") { return HttpStatus(408, desc); }
    static inline HttpStatus Conflict(const arduino::String &desc = "") { return HttpStatus(409, desc); }
    static inline HttpStatus Gone(const arduino::String &desc = "") { return HttpStatus(410, desc); }
    static inline HttpStatus LengthRequired(const arduino::String &desc = "") { return HttpStatus(411, desc); }
    static inline HttpStatus PreconditionFailed(const arduino::String &desc = "") { return HttpStatus(412, desc); }
    static inline HttpStatus PayloadTooLarge(const arduino::String &desc = "") { return HttpStatus(413, desc); }
    static inline HttpStatus UriTooLong(const arduino::String &desc = "") { return HttpStatus(414, desc); }
    static inline HttpStatus UnsupportedMediaType(const arduino::String &desc = "") { return HttpStatus(415, desc); }
    static inline HttpStatus RangeNotSatisfiable(const arduino::String &desc = "") { return HttpStatus(416, desc); }
    static inline HttpStatus ExpectationFailed(const arduino::String &desc = "") { return HttpStatus(417, desc); }
    static inline HttpStatus ImATeapot(const arduino::String &desc = "") { return HttpStatus(418, desc); }
    static inline HttpStatus MisdirectedRequest(const arduino::String &desc = "") { return HttpStatus(421, desc); }
    static inline HttpStatus UnprocessableEntity(const arduino::String &desc = "") { return HttpStatus(422, desc); }
    static inline HttpStatus Locked(const arduino::String &desc = "") { return HttpStatus(423, desc); }
    static inline HttpStatus FailedDependency(const arduino::String &desc = "") { return HttpStatus(424, desc); }
    static inline HttpStatus TooEarly(const arduino::String &desc = "") { return HttpStatus(425, desc); }
    static inline HttpStatus UpgradeRequired(const arduino::String &desc = "") { return HttpStatus(426, desc); }
    static inline HttpStatus PreconditionRequired(const arduino::String &desc = "") { return HttpStatus(428, desc); }
    static inline HttpStatus TooManyRequests(const arduino::String &desc = "") { return HttpStatus(429, desc); }
    static inline HttpStatus RequestHeaderFieldsTooLarge(const arduino::String &desc = "") { return HttpStatus(431, desc); }
    static inline HttpStatus UnavailableForLegalReasons(const arduino::String &desc = "") { return HttpStatus(451, desc); }
    static inline HttpStatus InternalServerError(const arduino::String &desc = "") { return HttpStatus(500, desc); }
    static inline HttpStatus NotImplemented(const arduino::String &desc = "") { return HttpStatus(501, desc); }
    static inline HttpStatus BadGateway(const arduino::String &desc = "") { return HttpStatus(502, desc); }
    static inline HttpStatus ServiceUnavailable(const arduino::String &desc = "") { return HttpStatus(503, desc); }
    static inline HttpStatus GatewayTimeout(const arduino::String &desc = "") { return HttpStatus(504, desc); }
    static inline HttpStatus HttpVersionNotSupported(const arduino::String &desc = "") { return HttpStatus(505, desc); }
    static inline HttpStatus VariantAlsoNegotiates(const arduino::String &desc = "") { return HttpStatus(506, desc); }
    static inline HttpStatus InsufficientStorage(const arduino::String &desc = "") { return HttpStatus(507, desc); }
    static inline HttpStatus LoopDetected(const arduino::String &desc = "") { return HttpStatus(508, desc); }
    static inline HttpStatus NotExtended(const arduino::String &desc = "") { return HttpStatus(510, desc); }
    static inline HttpStatus NetworkAuthenticationRequired(const arduino::String &desc = "") { return HttpStatus(511, desc); }

  private:
    uint16_t value_;
    arduino::String description_;
  };

} // namespace HttpServerAdvanced
