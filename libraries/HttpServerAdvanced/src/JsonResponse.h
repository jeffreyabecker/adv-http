#pragma once
#include <initializer_list>
#include "./HttpStatus.h"
#include "./HttpHeader.h"
#include "./IHttpResponse.h"
#include <Arduino.h>
#include <memory>
#include <ArduinoJson.h>

namespace HttpServerAdvanced
{

    class JsonResponse
    {
    public:
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const JsonDocument &doc, std::initializer_list<HttpHeader> headers = {});
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const JsonDocument &doc);
    };
}
