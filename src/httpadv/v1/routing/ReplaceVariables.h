#pragma once

#include "../transport/ByteStream.h"
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

namespace httpadv::v1::routing
{
    using httpadv::v1::core::HttpHeader;
    using httpadv::v1::core::HttpHeaderCollection;
    using httpadv::v1::core::HttpHeaderNames;
    using httpadv::v1::core::HttpStatus;
    using httpadv::v1::handlers::IHttpHandler;
    using httpadv::v1::response::IHttpResponse;
    using httpadv::v1::response::HttpResponse;
    using lumalink::platform::buffers::AvailableBytes;
    using lumalink::platform::buffers::AvailableResult;
    using lumalink::platform::buffers::ExhaustedResult;
    using lumalink::platform::buffers::IByteSink;
    using lumalink::platform::buffers::IByteSource;

#ifndef HTTPSERVER_ADVANCED_REPLACE_VARIABLES_MAX_TOKEN_BYTES
#define HTTPSERVER_ADVANCED_REPLACE_VARIABLES_MAX_TOKEN_BYTES 128
#endif
    static constexpr std::size_t ReplaceVariablesMaxTokenBytes = HTTPSERVER_ADVANCED_REPLACE_VARIABLES_MAX_TOKEN_BYTES;

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

    namespace ReplaceVariablesDetail
    {
        inline char ToLowerAscii(char value)
        {
            if (value >= 'A' && value <= 'Z')
            {
                return static_cast<char>(value - 'A' + 'a');
            }
            return value;
        }

        inline std::string_view TrimAscii(std::string_view input)
        {
            while (!input.empty() && (input.front() == ' ' || input.front() == '\t' || input.front() == '\r' || input.front() == '\n'))
            {
                input.remove_prefix(1);
            }
            while (!input.empty() && (input.back() == ' ' || input.back() == '\t' || input.back() == '\r' || input.back() == '\n'))
            {
                input.remove_suffix(1);
            }
            return input;
        }

        inline std::string ToLowerAsciiString(std::string_view input)
        {
            std::string lower;
            lower.reserve(input.size());
            for (char value : input)
            {
                lower.push_back(ToLowerAscii(value));
            }
            return lower;
        }

        inline bool StartsWith(std::string_view value, std::string_view prefix)
        {
            return value.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), value.begin());
        }

        inline bool EndsWith(std::string_view value, std::string_view suffix)
        {
            return value.size() >= suffix.size() &&
                   std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
        }

        inline bool IsTextLikeContentType(std::string_view contentType)
        {
            std::string_view normalized = TrimAscii(contentType);
            const std::size_t semicolonIndex = normalized.find(';');
            if (semicolonIndex != std::string_view::npos)
            {
                normalized = normalized.substr(0, semicolonIndex);
            }
            normalized = TrimAscii(normalized);

            if (normalized.empty())
            {
                return true;
            }

            const std::string lower = ToLowerAsciiString(normalized);
            if (StartsWith(lower, "text/"))
            {
                return true;
            }
            if (lower == "application/json" || lower == "application/javascript" ||
                lower == "application/xml" || lower == "application/xhtml+xml")
            {
                return true;
            }
            if (EndsWith(lower, "+json") || EndsWith(lower, "+xml"))
            {
                return true;
            }

            return false;
        }

        inline bool HasUnsupportedContentEncoding(const HttpHeaderCollection &headers)
        {
            const std::optional<HttpHeader> header = headers.find(HttpHeaderNames::ContentEncoding);
            if (!header.has_value())
            {
                return false;
            }

            const std::string value = ToLowerAsciiString(TrimAscii(header->valueView()));
            return !value.empty() && value != "identity";
        }

        inline bool HasUnsupportedTransferEncoding(const HttpHeaderCollection &headers)
        {
            const std::optional<HttpHeader> header = headers.find(HttpHeaderNames::TransferEncoding);
            if (!header.has_value())
            {
                return false;
            }

            const std::string value = ToLowerAsciiString(TrimAscii(header->valueView()));
            return value != "chunked";
        }

        inline bool StatusHasNoBody(HttpStatus status)
        {
            return status == HttpStatus::NoContent() ||
                   status == HttpStatus::NotModified() ||
                   (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
        }

        class StringByteSink : public IByteSink
        {
        public:
            explicit StringByteSink(std::string &storage)
                : storage_(storage)
            {
            }

            std::size_t write(lumalink::span<const std::uint8_t> buffer) override
            {
                storage_.append(reinterpret_cast<const char *>(buffer.data()), buffer.size());
                return buffer.size();
            }

            void flush() override
            {
            }

        private:
            std::string &storage_;
        };

        class ReplaceVariablesByteSource : public IByteSource
        {
        private:
            enum class ParserState
            {
                CopyLiteral,
                ReadToken
            };

            std::unique_ptr<IByteSource> inner_;
            ReplaceVariablesValueWriter resolver_;
            ReplaceVariablesOptions options_;

            std::string output_;
            std::size_t outputOffset_ = 0;

            ParserState parserState_ = ParserState::CopyLiteral;
            std::string prefixMatch_;
            std::string tokenBuffer_;
            std::string suffixMatch_;

            bool streamExhausted_ = false;

            std::size_t outputSize() const
            {
                return output_.size() - outputOffset_;
            }

            void compactOutput()
            {
                if (outputOffset_ == 0)
                {
                    return;
                }

                if (outputOffset_ >= output_.size())
                {
                    output_.clear();
                    outputOffset_ = 0;
                    return;
                }

                output_.erase(0, outputOffset_);
                outputOffset_ = 0;
            }

            void appendByte(char value)
            {
                output_.push_back(value);
            }

            void appendText(std::string_view value)
            {
                output_.append(value.data(), value.size());
            }

            bool prefixMatchesCurrentWindow() const
            {
                if (prefixMatch_.size() > options_.prefix.size())
                {
                    return false;
                }
                return std::equal(prefixMatch_.begin(), prefixMatch_.end(), options_.prefix.begin());
            }

            void consumeLiteralByte(char value)
            {
                if (options_.prefix.empty())
                {
                    appendByte(value);
                    return;
                }

                prefixMatch_.push_back(value);
                while (!prefixMatch_.empty() && !prefixMatchesCurrentWindow())
                {
                    appendByte(prefixMatch_.front());
                    prefixMatch_.erase(prefixMatch_.begin());
                }

                if (prefixMatch_.size() == options_.prefix.size())
                {
                    prefixMatch_.clear();
                    tokenBuffer_.clear();
                    suffixMatch_.clear();
                    parserState_ = ParserState::ReadToken;
                }
            }

            void emitMissingToken(std::string_view key)
            {
                if (options_.missingValuePolicy == ReplaceVariablesMissingValuePolicy::ReplaceWithEmpty)
                {
                    return;
                }

                appendText(options_.prefix);
                appendText(key);
                appendText(options_.suffix);
            }

            void closeToken()
            {
                parserState_ = ParserState::CopyLiteral;

                std::string replacement;
                bool found = false;
                if (resolver_)
                {
                    StringByteSink sink(replacement);
                    found = resolver_(std::string_view(tokenBuffer_.data(), tokenBuffer_.size()), sink);
                }

                if (found)
                {
                    appendText(replacement);
                }
                else
                {
                    emitMissingToken(std::string_view(tokenBuffer_.data(), tokenBuffer_.size()));
                }

                tokenBuffer_.clear();
                suffixMatch_.clear();
            }

            void abortTokenAsLiteral()
            {
                parserState_ = ParserState::CopyLiteral;
                appendText(options_.prefix);
                appendText(tokenBuffer_);
                appendText(suffixMatch_);
                tokenBuffer_.clear();
                suffixMatch_.clear();
            }

            void consumeTokenByte(char value)
            {
                if (options_.suffix.empty())
                {
                    appendText(options_.prefix);
                    appendText(tokenBuffer_);
                    appendByte(value);
                    tokenBuffer_.clear();
                    suffixMatch_.clear();
                    parserState_ = ParserState::CopyLiteral;
                    return;
                }

                const bool matchesSuffix = value == options_.suffix[suffixMatch_.size()];
                if (matchesSuffix)
                {
                    suffixMatch_.push_back(value);
                    if (suffixMatch_.size() == options_.suffix.size())
                    {
                        closeToken();
                    }
                }
                else
                {
                    if (!suffixMatch_.empty())
                    {
                        tokenBuffer_.append(suffixMatch_);
                        suffixMatch_.clear();
                    }

                    if (!options_.suffix.empty() && value == options_.suffix.front())
                    {
                        suffixMatch_.push_back(value);
                        if (suffixMatch_.size() == options_.suffix.size())
                        {
                            closeToken();
                        }
                    }
                    else
                    {
                        tokenBuffer_.push_back(value);
                    }
                }

                if (tokenBuffer_.size() + suffixMatch_.size() > options_.maxTokenBytes)
                {
                    abortTokenAsLiteral();
                }
            }

            void consumeByte(char value)
            {
                if (parserState_ == ParserState::ReadToken)
                {
                    consumeTokenByte(value);
                    return;
                }

                consumeLiteralByte(value);
            }

            void flushPendingParserState()
            {
                if (parserState_ == ParserState::ReadToken)
                {
                    appendText(options_.prefix);
                    appendText(tokenBuffer_);
                    appendText(suffixMatch_);
                    tokenBuffer_.clear();
                    suffixMatch_.clear();
                    parserState_ = ParserState::CopyLiteral;
                }

                if (!prefixMatch_.empty())
                {
                    appendText(prefixMatch_);
                    prefixMatch_.clear();
                }
            }

            void fillOutput(std::size_t targetBytes)
            {
                if (!inner_)
                {
                    streamExhausted_ = true;
                    return;
                }

                while (outputSize() < targetBytes && !streamExhausted_)
                {
                    std::uint8_t readBuffer[64] = {};
                    const std::size_t readCount = inner_->read(lumalink::span<std::uint8_t>(readBuffer, sizeof(readBuffer)));
                    if (readCount == 0)
                    {
                        const AvailableResult available = inner_->available();
                        if (available.isExhausted())
                        {
                            flushPendingParserState();
                            streamExhausted_ = true;
                        }
                        break;
                    }

                    for (std::size_t i = 0; i < readCount; ++i)
                    {
                        consumeByte(static_cast<char>(readBuffer[i]));
                    }
                }
            }

        public:
            ReplaceVariablesByteSource(std::unique_ptr<IByteSource> inner,
                                       ReplaceVariablesValueWriter resolver,
                                       ReplaceVariablesOptions options)
                : inner_(std::move(inner)),
                  resolver_(std::move(resolver)),
                  options_(std::move(options))
            {
            }

            AvailableResult available() override
            {
                if (outputSize() > 0)
                {
                    return AvailableBytes(outputSize());
                }

                fillOutput(1);
                if (outputSize() > 0)
                {
                    return AvailableBytes(outputSize());
                }

                if (streamExhausted_)
                {
                    return ExhaustedResult();
                }

                return inner_ ? inner_->available() : ExhaustedResult();
            }

            std::size_t read(lumalink::span<std::uint8_t> buffer) override
            {
                if (buffer.empty())
                {
                    return 0;
                }

                fillOutput(buffer.size());
                const std::size_t copied = (std::min)(buffer.size(), outputSize());
                if (copied == 0)
                {
                    return 0;
                }

                std::copy_n(reinterpret_cast<const std::uint8_t *>(output_.data() + outputOffset_), copied, buffer.data());
                outputOffset_ += copied;
                compactOutput();
                return copied;
            }

            std::size_t peek(lumalink::span<std::uint8_t> buffer) override
            {
                if (buffer.empty())
                {
                    return 0;
                }

                fillOutput(buffer.size());
                const std::size_t copied = (std::min)(buffer.size(), outputSize());
                if (copied == 0)
                {
                    return 0;
                }

                std::copy_n(reinterpret_cast<const std::uint8_t *>(output_.data() + outputOffset_), copied, buffer.data());
                return copied;
            }
        };

        inline bool shouldApplyFilter(IHttpResponse &response, const ReplaceVariablesOptions &options)
        {
            if (options.prefix.empty() || options.suffix.empty() || options.maxTokenBytes == 0)
            {
                return false;
            }

            if (StatusHasNoBody(response.status()))
            {
                return false;
            }

            const HttpHeaderCollection &headers = response.headers();
            if (HasUnsupportedContentEncoding(headers) || HasUnsupportedTransferEncoding(headers))
            {
                return false;
            }

            if (!options.restrictToTextLikeContentTypes)
            {
                return true;
            }

            const std::optional<HttpHeader> contentType = headers.find(HttpHeaderNames::ContentType);
            if (!contentType.has_value())
            {
                return true;
            }

            return IsTextLikeContentType(contentType->valueView());
        }
    } // namespace ReplaceVariablesDetail

    inline HttpResponse::ResponseFilter ReplaceVariables(ReplaceVariablesValueWriter resolver, ReplaceVariablesOptions options = {})
    {
        return [resolver = std::move(resolver), options = std::move(options)](std::unique_ptr<IHttpResponse> response) mutable -> std::unique_ptr<IHttpResponse>
        {
            if (!response || !ReplaceVariablesDetail::shouldApplyFilter(*response, options))
            {
                return response;
            }

            std::unique_ptr<IByteSource> body = response->getBody();
            if (!body)
            {
                return response;
            }

            HttpHeaderCollection headers = response->headers();
            headers.remove(HttpHeaderNames::ContentLength);
            headers.set(HttpHeaderNames::TransferEncoding, "chunked", true);

            return std::make_unique<HttpResponse>(
                response->status(),
                std::make_unique<ReplaceVariablesDetail::ReplaceVariablesByteSource>(
                    std::move(body),
                    resolver,
                    options),
                std::move(headers));
        };
    }

    inline HttpResponse::ResponseFilter ReplaceVariables(const std::vector<std::pair<std::string, std::string>> &values, ReplaceVariablesOptions options = {})
    {
        std::vector<std::pair<std::string, std::string>> captured = values;
        ReplaceVariablesValueWriter resolver = [captured = std::move(captured)](std::string_view key, IByteSink &output) -> bool
        {
            auto found = std::find_if(captured.begin(), captured.end(), [key](const std::pair<std::string, std::string> &entry)
            {
                return entry.first == key;
            });
            if (found == captured.end())
            {
                return false;
            }

            output.write(std::string_view(found->second.data(), found->second.size()));
            return true;
        };

        return ReplaceVariables(std::move(resolver), std::move(options));
    }

} // namespace httpadv::v1::routing
