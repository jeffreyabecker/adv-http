#pragma once
#include <Arduino.h>
#include "../HttpHandler.h"
#include "./HandlerMatcher.h"
namespace HttpServerAdvanced::Core
{
    using InvocationParams = std::vector<String>;
    using ParameterExtractor = std::function<InvocationParams(HttpContext &context)>;

    namespace Detail
    {
        inline InvocationParams EmptyParameters(HttpContext &context)
        {
            return {};
        }
    }

    inline std::function<InvocationParams(HttpServerAdvanced::Core::HttpContext &)> ParametersFromUri(HandlerMatcher uri)
    {
        return [uri](HttpContext &context)
        {
            return uri.extractParameters(context);
        };
    }
} // namespace HttpServerAdvanced::Core
