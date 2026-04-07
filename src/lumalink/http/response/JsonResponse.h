#pragma once
#include "../core/Defines.h"
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
#include <initializer_list>
#include <memory>
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "IHttpResponse.h"
#include <ArduinoJson.h>

namespace lumalink::http::response
{

    class JsonResponse
    {
    public:
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const JsonDocument &doc, std::initializer_list<lumalink::http::core::HttpHeader> headers );
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const JsonDocument &doc);
    };
}

#endif