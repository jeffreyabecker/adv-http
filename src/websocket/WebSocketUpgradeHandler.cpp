#include "WebSocketUpgradeHandler.h"

#include "WebSocketSessionRuntime.h"

#include "../compat/Span.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../core/HttpRequest.h"
#include "../core/HttpStatus.h"
#include "../util/HttpUtility.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace HttpServerAdvanced
{
    namespace
    {
        static constexpr std::string_view WebSocketUpgradeToken = "websocket";
        static constexpr std::string_view UpgradeToken = "upgrade";
        static constexpr std::string_view SupportedWebSocketVersion = "13";
        static constexpr std::string_view WebSocketGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        char toLowerAscii(char value)
        {
            if (value >= 'A' && value <= 'Z')
            {
                return static_cast<char>(value - 'A' + 'a');
            }

            return value;
        }

        std::string toLowerAsciiString(std::string_view value)
        {
            std::string lowered;
            lowered.reserve(value.size());
            for (const char ch : value)
            {
                lowered.push_back(toLowerAscii(ch));
            }
            return lowered;
        }

        std::string trimAsciiWhitespace(std::string_view value)
        {
            std::size_t begin = 0;
            std::size_t end = value.size();

            while (begin < end && (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\r' || value[begin] == '\n'))
            {
                ++begin;
            }

            while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r' || value[end - 1] == '\n'))
            {
                --end;
            }

            return std::string(value.substr(begin, end - begin));
        }

        std::vector<std::string> splitHeaderTokens(std::string_view value)
        {
            std::vector<std::string> tokens;
            std::size_t begin = 0;
            while (begin <= value.size())
            {
                const std::size_t commaPos = value.find(',', begin);
                const std::size_t end = commaPos == std::string_view::npos ? value.size() : commaPos;
                std::string token = trimAsciiWhitespace(value.substr(begin, end - begin));
                if (!token.empty())
                {
                    tokens.push_back(std::move(token));
                }

                if (commaPos == std::string_view::npos)
                {
                    break;
                }

                begin = commaPos + 1;
            }

            return tokens;
        }

        bool containsTokenCaseInsensitive(std::string_view value, std::string_view expectedToken)
        {
            const std::vector<std::string> tokens = splitHeaderTokens(value);
            const std::string expectedLower = toLowerAsciiString(expectedToken);
            for (const auto &token : tokens)
            {
                if (toLowerAsciiString(token) == expectedLower)
                {
                    return true;
                }
            }
            return false;
        }

        bool hasConflictingUpgradeHeaderValue(std::string_view value)
        {
            const std::vector<std::string> tokens = splitHeaderTokens(value);
            if (tokens.size() != 1)
            {
                return true;
            }

            return toLowerAsciiString(tokens.front()) != WebSocketUpgradeToken;
        }

        std::array<std::uint8_t, 20> sha1Digest(std::string_view input)
        {
            std::uint64_t bitLength = static_cast<std::uint64_t>(input.size()) * 8ULL;
            std::vector<std::uint8_t> message(input.begin(), input.end());
            message.push_back(0x80);

            while ((message.size() % 64) != 56)
            {
                message.push_back(0x00);
            }

            for (int shift = 56; shift >= 0; shift -= 8)
            {
                message.push_back(static_cast<std::uint8_t>((bitLength >> static_cast<std::uint64_t>(shift)) & 0xFFU));
            }

            std::uint32_t h0 = 0x67452301U;
            std::uint32_t h1 = 0xEFCDAB89U;
            std::uint32_t h2 = 0x98BADCFEU;
            std::uint32_t h3 = 0x10325476U;
            std::uint32_t h4 = 0xC3D2E1F0U;

            auto rotateLeft = [](std::uint32_t value, std::uint32_t bits) -> std::uint32_t
            {
                return static_cast<std::uint32_t>((value << bits) | (value >> (32U - bits)));
            };

            for (std::size_t chunkOffset = 0; chunkOffset < message.size(); chunkOffset += 64)
            {
                std::uint32_t words[80] = {};
                for (std::size_t i = 0; i < 16; ++i)
                {
                    const std::size_t base = chunkOffset + i * 4;
                    words[i] = (static_cast<std::uint32_t>(message[base]) << 24U) |
                               (static_cast<std::uint32_t>(message[base + 1]) << 16U) |
                               (static_cast<std::uint32_t>(message[base + 2]) << 8U) |
                               static_cast<std::uint32_t>(message[base + 3]);
                }

                for (std::size_t i = 16; i < 80; ++i)
                {
                    words[i] = rotateLeft(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1U);
                }

                std::uint32_t a = h0;
                std::uint32_t b = h1;
                std::uint32_t c = h2;
                std::uint32_t d = h3;
                std::uint32_t e = h4;

                for (std::size_t i = 0; i < 80; ++i)
                {
                    std::uint32_t f = 0;
                    std::uint32_t k = 0;

                    if (i < 20)
                    {
                        f = (b & c) | ((~b) & d);
                        k = 0x5A827999U;
                    }
                    else if (i < 40)
                    {
                        f = b ^ c ^ d;
                        k = 0x6ED9EBA1U;
                    }
                    else if (i < 60)
                    {
                        f = (b & c) | (b & d) | (c & d);
                        k = 0x8F1BBCDCU;
                    }
                    else
                    {
                        f = b ^ c ^ d;
                        k = 0xCA62C1D6U;
                    }

                    std::uint32_t temp = rotateLeft(a, 5U) + f + e + k + words[i];
                    e = d;
                    d = c;
                    c = rotateLeft(b, 30U);
                    b = a;
                    a = temp;
                }

                h0 += a;
                h1 += b;
                h2 += c;
                h3 += d;
                h4 += e;
            }

            std::array<std::uint8_t, 20> digest = {};
            const std::uint32_t words[5] = {h0, h1, h2, h3, h4};
            for (std::size_t i = 0; i < 5; ++i)
            {
                digest[i * 4] = static_cast<std::uint8_t>((words[i] >> 24U) & 0xFFU);
                digest[i * 4 + 1] = static_cast<std::uint8_t>((words[i] >> 16U) & 0xFFU);
                digest[i * 4 + 2] = static_cast<std::uint8_t>((words[i] >> 8U) & 0xFFU);
                digest[i * 4 + 3] = static_cast<std::uint8_t>(words[i] & 0xFFU);
            }

            return digest;
        }

        std::string createWebSocketAcceptValue(std::string_view secWebSocketKey)
        {
            std::string source;
            source.reserve(secWebSocketKey.size() + WebSocketGuid.size());
            source.append(secWebSocketKey);
            source.append(WebSocketGuid);

            const std::array<std::uint8_t, 20> digest = sha1Digest(source);
            return WebUtility::Base64Encode(digest.data(), digest.size(), false);
        }

        std::optional<WebSocketUpgradeHandler::UpgradeFailure> validateUpgradeRequest(const HttpRequest &request, std::string &normalizedKey)
        {
            if (request.methodView() != "GET")
            {
                return WebSocketUpgradeHandler::UpgradeFailure::InvalidMethod;
            }

            const auto connectionHeader = request.headers().find(HttpHeaderNames::Connection);
            if (!connectionHeader.has_value() || !containsTokenCaseInsensitive(connectionHeader->valueView(), UpgradeToken))
            {
                return WebSocketUpgradeHandler::UpgradeFailure::MissingUpgradeIntent;
            }

            const auto upgradeHeader = request.headers().find(HttpHeaderNames::Upgrade);
            if (!upgradeHeader.has_value())
            {
                return WebSocketUpgradeHandler::UpgradeFailure::MissingUpgradeIntent;
            }

            if (hasConflictingUpgradeHeaderValue(upgradeHeader->valueView()))
            {
                return WebSocketUpgradeHandler::UpgradeFailure::ConflictingHeaders;
            }

            const auto versionHeader = request.headers().find(HttpHeaderNames::SecWebSocketVersion);
            if (!versionHeader.has_value())
            {
                return WebSocketUpgradeHandler::UpgradeFailure::UnsupportedVersion;
            }

            const std::string normalizedVersion = trimAsciiWhitespace(versionHeader->valueView());
            if (normalizedVersion.find(',') != std::string::npos || normalizedVersion != SupportedWebSocketVersion)
            {
                return WebSocketUpgradeHandler::UpgradeFailure::UnsupportedVersion;
            }

            const auto keyHeader = request.headers().find(HttpHeaderNames::SecWebSocketKey);
            if (!keyHeader.has_value())
            {
                return WebSocketUpgradeHandler::UpgradeFailure::InvalidKeyLength;
            }

            normalizedKey = trimAsciiWhitespace(keyHeader->valueView());
            if (normalizedKey.find(',') != std::string::npos || normalizedKey.size() != 24U)
            {
                return WebSocketUpgradeHandler::UpgradeFailure::InvalidKeyLength;
            }

            const std::vector<std::uint8_t> decodedKey = WebUtility::Base64Decode(normalizedKey, false);
            if (decodedKey.size() != 16U)
            {
                return WebSocketUpgradeHandler::UpgradeFailure::MalformedKeyText;
            }

            if (WebUtility::Base64Encode(decodedKey.data(), decodedKey.size(), false) != normalizedKey)
            {
                return WebSocketUpgradeHandler::UpgradeFailure::MalformedKeyText;
            }

            return std::nullopt;
        }

        std::string createHandshakeResponseText(std::string_view acceptValue)
        {
            std::string response;
            response.reserve(160);
            response.append("HTTP/1.1 101 Switching Protocols\r\n");
            response.append("Upgrade: websocket\r\n");
            response.append("Connection: Upgrade\r\n");
            response.append("Sec-WebSocket-Accept: ");
            response.append(acceptValue);
            response.append("\r\n\r\n");
            return response;
        }
    }

    bool WebSocketUpgradeHandler::isWebSocketUpgradeCandidate(const HttpRequest &request)
    {
        const HttpHeaderCollection &headers = request.headers();
        if (headers.exists(HttpHeaderNames::SecWebSocketKey) ||
            headers.exists(HttpHeaderNames::SecWebSocketVersion) ||
            headers.exists(HttpHeaderNames::SecWebSocketProtocol))
        {
            return true;
        }

        const auto upgradeHeader = headers.find(HttpHeaderNames::Upgrade);
        if (upgradeHeader.has_value() && containsTokenCaseInsensitive(upgradeHeader->valueView(), WebSocketUpgradeToken))
        {
            return true;
        }

        const auto connectionHeader = headers.find(HttpHeaderNames::Connection);
        if (connectionHeader.has_value() && containsTokenCaseInsensitive(connectionHeader->valueView(), UpgradeToken))
        {
            return true;
        }

        return false;
    }

    HandlerResult WebSocketUpgradeHandler::handle(HttpRequest &request, const WebSocketCallbacks &callbacks) const
    {
        std::string key;
        const std::optional<UpgradeFailure> failure = validateUpgradeRequest(request, key);
        if (failure.has_value())
        {
            return rejectUpgrade(request, *failure);
        }

        const std::string acceptValue = createWebSocketAcceptValue(key);
        auto session = std::make_unique<WebSocketSessionRuntime>(createHandshakeResponseText(acceptValue), callbacks);
        return HandlerResult::upgradeResult(std::move(session));
    }

    HandlerResult WebSocketUpgradeHandler::rejectUpgrade(HttpRequest &request, UpgradeFailure failure)
    {
        HttpStatus status = HttpStatus::BadRequest();
        std::string message = "WebSocket upgrade rejected: malformed headers or key";

        switch (failure)
        {
        case UpgradeFailure::InvalidMethod:
            status = HttpStatus::MethodNotAllowed();
            message = "WebSocket upgrade rejected: method must be GET";
            break;
        case UpgradeFailure::MissingUpgradeIntent:
            status = HttpStatus::UpgradeRequired();
            message = "WebSocket upgrade rejected: missing upgrade intent";
            break;
        case UpgradeFailure::UnsupportedVersion:
            status = HttpStatus::BadRequest();
            message = "WebSocket upgrade rejected: unsupported version";
            break;
        case UpgradeFailure::InvalidKeyLength:
            status = HttpStatus::BadRequest();
            message = "WebSocket upgrade rejected: invalid key length";
            break;
        case UpgradeFailure::MalformedKeyText:
            status = HttpStatus::BadRequest();
            message = "WebSocket upgrade rejected: malformed key";
            break;
        case UpgradeFailure::ConflictingHeaders:
            status = HttpStatus::BadRequest();
            message = "WebSocket upgrade rejected: conflicting headers";
            break;
        }

        return HandlerResult::responseResult(request.createResponse(status, message));
    }
}
