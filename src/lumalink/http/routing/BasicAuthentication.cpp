#include "BasicAuthentication.h"
#include <format>

namespace lumalink::http::routing
{
    std::unique_ptr<IHttpResponse> defaultOnFailure(HttpRequestContext &, std::string_view realm)
    {
        return StringResponse::create(
            HttpStatus::Unauthorized(),
            std::string_view("Unauthorized"),
            {HttpHeader(std::string_view(HttpHeaderNames::WwwAuthenticate), std::format("Basic realm=\"{}\"", realm))});
    }
} // namespace lumalink::http::routing
