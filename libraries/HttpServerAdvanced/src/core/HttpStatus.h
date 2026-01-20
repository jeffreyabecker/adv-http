#pragma once
// https://github.com/nodejs/llhttp
#include <Arduino.h>
#include <http_parser.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <algorithm>

namespace HttpServerAdvanced::Core
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
    static HttpStatus Continue(const arduino::String &desc = "");
    static HttpStatus SwitchingProtocols(const arduino::String &desc = "");
    static HttpStatus Processing(const arduino::String &desc = "");
    static HttpStatus Ok(const arduino::String &desc = "");
    static HttpStatus Created(const arduino::String &desc = "");
    static HttpStatus Accepted(const arduino::String &desc = "");
    static HttpStatus NonAuthoritativeInformation(const arduino::String &desc = "");
    static HttpStatus NoContent(const arduino::String &desc = "");
    static HttpStatus ResetContent(const arduino::String &desc = "");
    static HttpStatus PartialContent(const arduino::String &desc = "");
    static HttpStatus MultiStatus(const arduino::String &desc = "");
    static HttpStatus AlreadyReported(const arduino::String &desc = "");
    static HttpStatus ImUsed(const arduino::String &desc = "");
    static HttpStatus MultipleChoices(const arduino::String &desc = "");
    static HttpStatus MovedPermanently(const arduino::String &desc = "");
    static HttpStatus Found(const arduino::String &desc = "");
    static HttpStatus SeeOther(const arduino::String &desc = "");
    static HttpStatus NotModified(const arduino::String &desc = "");
    static HttpStatus UseProxy(const arduino::String &desc = "");
    static HttpStatus TemporaryRedirect(const arduino::String &desc = "");
    static HttpStatus PermanentRedirect(const arduino::String &desc = "");
    static HttpStatus BadRequest(const arduino::String &desc = "");
    static HttpStatus Unauthorized(const arduino::String &desc = "");
    static HttpStatus PaymentRequired(const arduino::String &desc = "");
    static HttpStatus Forbidden(const arduino::String &desc = "");
    static HttpStatus NotFound(const arduino::String &desc = "");
    static HttpStatus MethodNotAllowed(const arduino::String &desc = "");
    static HttpStatus NotAcceptable(const arduino::String &desc = "");
    static HttpStatus ProxyAuthenticationRequired(const arduino::String &desc = "");
    static HttpStatus RequestTimeout(const arduino::String &desc = "");
    static HttpStatus Conflict(const arduino::String &desc = "");
    static HttpStatus Gone(const arduino::String &desc = "");
    static HttpStatus LengthRequired(const arduino::String &desc = "");
    static HttpStatus PreconditionFailed(const arduino::String &desc = "");
    static HttpStatus PayloadTooLarge(const arduino::String &desc = "");
    static HttpStatus UriTooLong(const arduino::String &desc = "");
    static HttpStatus UnsupportedMediaType(const arduino::String &desc = "");
    static HttpStatus RangeNotSatisfiable(const arduino::String &desc = "");
    static HttpStatus ExpectationFailed(const arduino::String &desc = "");
    static HttpStatus ImATeapot(const arduino::String &desc = "");
    static HttpStatus MisdirectedRequest(const arduino::String &desc = "");
    static HttpStatus UnprocessableEntity(const arduino::String &desc = "");
    static HttpStatus Locked(const arduino::String &desc = "");
    static HttpStatus FailedDependency(const arduino::String &desc = "");
    static HttpStatus TooEarly(const arduino::String &desc = "");
    static HttpStatus UpgradeRequired(const arduino::String &desc = "");
    static HttpStatus PreconditionRequired(const arduino::String &desc = "");
    static HttpStatus TooManyRequests(const arduino::String &desc = "");
    static HttpStatus RequestHeaderFieldsTooLarge(const arduino::String &desc = "");
    static HttpStatus UnavailableForLegalReasons(const arduino::String &desc = "");
    static HttpStatus InternalServerError(const arduino::String &desc = "");
    static HttpStatus NotImplemented(const arduino::String &desc = "");
    static HttpStatus BadGateway(const arduino::String &desc = "");
    static HttpStatus ServiceUnavailable(const arduino::String &desc = "");
    static HttpStatus GatewayTimeout(const arduino::String &desc = "");
    static HttpStatus HttpVersionNotSupported(const arduino::String &desc = "");
    static HttpStatus VariantAlsoNegotiates(const arduino::String &desc = "");
    static HttpStatus InsufficientStorage(const arduino::String &desc = "");
    static HttpStatus LoopDetected(const arduino::String &desc = "");
    static HttpStatus NotExtended(const arduino::String &desc = "");
    static HttpStatus NetworkAuthenticationRequired(const arduino::String &desc = "");

  private:
    uint16_t value_;
    arduino::String description_;
  };

} // namespace HttpServerAdvanced
