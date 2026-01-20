
#include "./HttpStatus.h"
#include <Arduino.h>



namespace HttpServerAdvanced::Core
{

    HttpStatus HttpStatus::Continue(const arduino::String &desc)
    {
        return HttpStatus(100, desc);
    }

    HttpStatus HttpStatus::SwitchingProtocols(const arduino::String &desc)
    {
        return HttpStatus(101, desc);
    }

    HttpStatus HttpStatus::Processing(const arduino::String &desc)
    {
        return HttpStatus(102, desc);
    }

    HttpStatus HttpStatus::Ok(const arduino::String &desc)
    {
        return HttpStatus(200, desc);
    }

    HttpStatus HttpStatus::Created(const arduino::String &desc)
    {
        return HttpStatus(201, desc);
    }

    HttpStatus HttpStatus::Accepted(const arduino::String &desc)
    {
        return HttpStatus(202, desc);
    }

    HttpStatus HttpStatus::NonAuthoritativeInformation(const arduino::String &desc)
    {
        return HttpStatus(203, desc);
    }

    HttpStatus HttpStatus::NoContent(const arduino::String &desc)
    {
        return HttpStatus(204, desc);
    }

    HttpStatus HttpStatus::ResetContent(const arduino::String &desc)
    {
        return HttpStatus(205, desc);
    }

    HttpStatus HttpStatus::PartialContent(const arduino::String &desc)
    {
        return HttpStatus(206, desc);
    }

    HttpStatus HttpStatus::MultiStatus(const arduino::String &desc)
    {
        return HttpStatus(207, desc);
    }

    HttpStatus HttpStatus::AlreadyReported(const arduino::String &desc)
    {
        return HttpStatus(208, desc);
    }

    HttpStatus HttpStatus::ImUsed(const arduino::String &desc)
    {
        return HttpStatus(226, desc);
    }

    HttpStatus HttpStatus::MultipleChoices(const arduino::String &desc)
    {
        return HttpStatus(300, desc);
    }

    HttpStatus HttpStatus::MovedPermanently(const arduino::String &desc)
    {
        return HttpStatus(301, desc);
    }

    HttpStatus HttpStatus::Found(const arduino::String &desc)
    {
        return HttpStatus(302, desc);
    }

    HttpStatus HttpStatus::SeeOther(const arduino::String &desc)
    {
        return HttpStatus(303, desc);
    }

    HttpStatus HttpStatus::NotModified(const arduino::String &desc)
    {
        return HttpStatus(304, desc);
    }

    HttpStatus HttpStatus::UseProxy(const arduino::String &desc)
    {
        return HttpStatus(305, desc);
    }

    HttpStatus HttpStatus::TemporaryRedirect(const arduino::String &desc)
    {
        return HttpStatus(307, desc);
    }

    HttpStatus HttpStatus::PermanentRedirect(const arduino::String &desc)
    {
        return HttpStatus(308, desc);
    }

    HttpStatus HttpStatus::BadRequest(const arduino::String &desc)
    {
        return HttpStatus(400, desc);
    }

    HttpStatus HttpStatus::Unauthorized(const arduino::String &desc)
    {
        return HttpStatus(401, desc);
    }

    HttpStatus HttpStatus::PaymentRequired(const arduino::String &desc)
    {
        return HttpStatus(402, desc);
    }

    HttpStatus HttpStatus::Forbidden(const arduino::String &desc)
    {
        return HttpStatus(403, desc);
    }

    HttpStatus HttpStatus::NotFound(const arduino::String &desc)
    {
        return HttpStatus(404, desc);
    }

    HttpStatus HttpStatus::MethodNotAllowed(const arduino::String &desc)
    {
        return HttpStatus(405, desc);
    }

    HttpStatus HttpStatus::NotAcceptable(const arduino::String &desc)
    {
        return HttpStatus(406, desc);
    }

    HttpStatus HttpStatus::ProxyAuthenticationRequired(const arduino::String &desc)
    {
        return HttpStatus(407, desc);
    }

    HttpStatus HttpStatus::RequestTimeout(const arduino::String &desc)
    {
        return HttpStatus(408, desc);
    }

    HttpStatus HttpStatus::Conflict(const arduino::String &desc)
    {
        return HttpStatus(409, desc);
    }

    HttpStatus HttpStatus::Gone(const arduino::String &desc)
    {
        return HttpStatus(410, desc);
    }

    HttpStatus HttpStatus::LengthRequired(const arduino::String &desc)
    {
        return HttpStatus(411, desc);
    }

    HttpStatus HttpStatus::PreconditionFailed(const arduino::String &desc)
    {
        return HttpStatus(412, desc);
    }

    HttpStatus HttpStatus::PayloadTooLarge(const arduino::String &desc)
    {
        return HttpStatus(413, desc);
    }

    HttpStatus HttpStatus::UriTooLong(const arduino::String &desc)
    {
        return HttpStatus(414, desc);
    }

    HttpStatus HttpStatus::UnsupportedMediaType(const arduino::String &desc)
    {
        return HttpStatus(415, desc);
    }

    HttpStatus HttpStatus::RangeNotSatisfiable(const arduino::String &desc)
    {
        return HttpStatus(416, desc);
    }

    HttpStatus HttpStatus::ExpectationFailed(const arduino::String &desc)
    {
        return HttpStatus(417, desc);
    }

    HttpStatus HttpStatus::ImATeapot(const arduino::String &desc)
    {
        return HttpStatus(418, desc);
    }

    HttpStatus HttpStatus::MisdirectedRequest(const arduino::String &desc)
    {
        return HttpStatus(421, desc);
    }

    HttpStatus HttpStatus::UnprocessableEntity(const arduino::String &desc)
    {
        return HttpStatus(422, desc);
    }

    HttpStatus HttpStatus::Locked(const arduino::String &desc)
    {
        return HttpStatus(423, desc);
    }

    HttpStatus HttpStatus::FailedDependency(const arduino::String &desc)
    {
        return HttpStatus(424, desc);
    }

    HttpStatus HttpStatus::TooEarly(const arduino::String &desc)
    {
        return HttpStatus(425, desc);
    }

    HttpStatus HttpStatus::UpgradeRequired(const arduino::String &desc)
    {
        return HttpStatus(426, desc);
    }

    HttpStatus HttpStatus::PreconditionRequired(const arduino::String &desc)
    {
        return HttpStatus(428, desc);
    }

    HttpStatus HttpStatus::TooManyRequests(const arduino::String &desc)
    {
        return HttpStatus(429, desc);
    }

    HttpStatus HttpStatus::RequestHeaderFieldsTooLarge(const arduino::String &desc)
    {
        return HttpStatus(431, desc);
    }

    HttpStatus HttpStatus::UnavailableForLegalReasons(const arduino::String &desc)
    {
        return HttpStatus(451, desc);
    }

    HttpStatus HttpStatus::InternalServerError(const arduino::String &desc)
    {
        return HttpStatus(500, desc);
    }

    HttpStatus HttpStatus::NotImplemented(const arduino::String &desc)
    {
        return HttpStatus(501, desc);
    }

    HttpStatus HttpStatus::BadGateway(const arduino::String &desc)
    {
        return HttpStatus(502, desc);
    }

    HttpStatus HttpStatus::ServiceUnavailable(const arduino::String &desc)
    {
        return HttpStatus(503, desc);
    }

    HttpStatus HttpStatus::GatewayTimeout(const arduino::String &desc)
    {
        return HttpStatus(504, desc);
    }

    HttpStatus HttpStatus::HttpVersionNotSupported(const arduino::String &desc)
    {
        return HttpStatus(505, desc);
    }

    HttpStatus HttpStatus::VariantAlsoNegotiates(const arduino::String &desc)
    {
        return HttpStatus(506, desc);
    }

    HttpStatus HttpStatus::InsufficientStorage(const arduino::String &desc)
    {
        return HttpStatus(507, desc);
    }

    HttpStatus HttpStatus::LoopDetected(const arduino::String &desc)
    {
        return HttpStatus(508, desc);
    }

    HttpStatus HttpStatus::NotExtended(const arduino::String &desc)
    {
        return HttpStatus(510, desc);
    }

    HttpStatus HttpStatus::NetworkAuthenticationRequired(const arduino::String &desc)
    {
        return HttpStatus(511, desc);
    }

}