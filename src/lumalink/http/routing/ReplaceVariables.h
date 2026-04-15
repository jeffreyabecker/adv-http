#pragma once

#include "LumaLinkPlatform.h"
#include <span>
#include "../core/Defines.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../response/HttpResponse.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lumalink::http::routing
{
    using lumalink::http::core::HttpHeader;
    using lumalink::http::core::HttpHeaderCollection;
    using lumalink::http::core::HttpHeaderNames;
    using lumalink::http::core::HttpStatus;
    using lumalink::http::response::IHttpResponse;
    using lumalink::http::response::HttpResponse;
    using lumalink::platform::buffers::AvailableBytes;
    using lumalink::platform::buffers::ByteAvailability;
    using lumalink::platform::buffers::ExhaustedResult;
    using lumalink::platform::buffers::IsExhausted;
    using lumalink::platform::buffers::IByteSink;
    using lumalink::platform::buffers::IByteSource;

#ifndef LUMALINK_HTTP_REPLACE_VARIABLES_MAX_TOKEN_BYTES
#define LUMALINK_HTTP_REPLACE_VARIABLES_MAX_TOKEN_BYTES 128
#endif
    static constexpr std::size_t ReplaceVariablesMaxTokenBytes = LUMALINK_HTTP_REPLACE_VARIABLES_MAX_TOKEN_BYTES;

    enum class ReplaceVariablesMissingValuePolicy
    {
        KeepToken,
        ReplaceWithEmpty
    };

    struct ReplaceVariablesOptions
    {
        std::string prefix = "{{";
        std::string suffix = "}}";
        std::size_t maxTokenBytes = ReplaceVariablesMaxTokenBytes;
        bool restrictToTextLikeContentTypes = true;
        ReplaceVariablesMissingValuePolicy missingValuePolicy = ReplaceVariablesMissingValuePolicy::KeepToken;
    };

    using ReplaceVariablesValueWriter = std::function<bool(std::string_view key, IByteSink &output)>;

    HttpResponse::ResponseFilter ReplaceVariables(ReplaceVariablesValueWriter resolver, ReplaceVariablesOptions options = {});

    HttpResponse::ResponseFilter ReplaceVariables(const std::vector<std::pair<std::string, std::string>> &values, ReplaceVariablesOptions options = {});

} // namespace lumalink::http::routing
