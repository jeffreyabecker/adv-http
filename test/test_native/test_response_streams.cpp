#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/ByteStreamFixtures.h"

#include "../../src/lumalink/http/LumaLinkHttp.h"

#include <unity.h>

#include "../../src/lumalink/http/response/ChunkedHttpResponseBodyStream.h"
#include "../../src/lumalink/http/response/FormResponse.h"
#include "../../src/lumalink/http/response/HttpResponse.h"
#include "../../src/lumalink/http/response/StringResponse.h"
#include <lumalink/platform/buffers/ByteStream.h>
#include "../../src/lumalink/http/streams/Streams.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
#include <ArduinoJson.h>
#include "../../src/lumalink/http/response/JsonResponse.h"
#endif

using namespace lumalink::http::core;
using namespace lumalink::http::handlers;
using namespace lumalink::http::pipeline;
using namespace lumalink::http::response;
using namespace lumalink::http::routing;
using namespace lumalink::http::server;
using namespace lumalink::http::staticfiles;
using namespace lumalink::platform::buffers;
using namespace lumalink::platform::transport;
using namespace lumalink::platform::buffers;
using namespace lumalink::http::util;
using namespace lumalink::http::websocket;

namespace
{
    HttpHeaderCollection MakeDeterministicResponseHeaders()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeader::Date("Thu, 01 Jan 1970 00:00:00 GMT"));
        headers.set(HttpHeader::Server("UnitTest"));
        headers.set(HttpHeader::ContentType("text/plain"));
        headers.set(HttpHeader::Connection("close"));
        return headers;
    }

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_byte_source_reads_direct_body_content()
    {
        auto body = std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("ok"));

        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*body);
        TEST_ASSERT_EQUAL_STRING("ok", content.c_str());
    }

    void test_byte_source_reports_temporarily_unavailable()
    {
        auto body = std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(
            std::initializer_list<std::pair<const char *, bool>>{{"", true}});

        TEST_ASSERT_TRUE(body->available().isTemporarilyUnavailable());
    }

    void test_chunked_response_body_stream_reads_from_byte_source()
    {
        auto body = ChunkedHttpResponseBodyStream::create(
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("Hi")));

        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*body);
        TEST_ASSERT_EQUAL_STRING("2\r\nHi\r\n0\r\n\r\n", content.c_str());
    }

    void test_chunked_response_body_stream_emits_final_terminator_for_empty_source()
    {
        auto body = ChunkedHttpResponseBodyStream::create(std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>());

        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*body);
        TEST_ASSERT_EQUAL_STRING("0\r\n\r\n", content.c_str());
        TEST_ASSERT_TRUE(body->available().isExhausted());

        std::uint8_t byte = 0;
        TEST_ASSERT_EQUAL_UINT64(0, body->read(lumalink::span<std::uint8_t>(&byte, 1)));
    }

    void test_chunked_response_body_stream_preserves_varying_source_chunk_sizes()
    {
        auto body = ChunkedHttpResponseBodyStream::create(
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"A", false}, {"BCD", false}, {"EF", false}}));

        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*body);
        TEST_ASSERT_EQUAL_STRING("1\r\nA\r\n3\r\nBCD\r\n2\r\nEF\r\n0\r\n\r\n", content.c_str());
    }

    void test_chunked_response_body_stream_splits_large_available_span_at_frame_boundary()
    {
        const std::string payload(lumalink::http::core::ETHERNET_FRAME_BUFFER_SIZE + 3, 'a');
        auto body = ChunkedHttpResponseBodyStream::create(
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText(payload)));

        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*body);

        std::string expected;
        expected.reserve(payload.size() + 32);
        expected += "59c\r\n";
        expected.append(lumalink::http::core::ETHERNET_FRAME_BUFFER_SIZE, 'a');
        expected += "\r\n3\r\n";
        expected.append(3, 'a');
        expected += "\r\n0\r\n\r\n";

        TEST_ASSERT_EQUAL_STRING(expected.c_str(), content.c_str());
    }

    void test_http_response_accepts_byte_source_body()
    {
        HttpHeaderCollection headers;
        HttpResponse response(HttpStatus::Ok(), std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("body")), std::move(headers));

        auto body = response.getBody();
        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*body);
        TEST_ASSERT_EQUAL_STRING("body", content.c_str());
    }

    void test_http_response_returns_status_and_mutable_headers()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeader::ContentType("application/custom"));

        HttpResponse response(HttpStatus::Accepted(), std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("body")), std::move(headers));

        TEST_ASSERT_EQUAL(202, static_cast<int>(response.status()));
        TEST_ASSERT_TRUE(response.headers().exists(HttpHeaderNames::ContentType, "application/custom"));

        response.headers().set(HttpHeader::Connection("keep-alive"));
        TEST_ASSERT_TRUE(response.headers().exists(HttpHeaderNames::Connection, "keep-alive"));
    }

    void test_http_response_transfers_body_ownership_once()
    {
        HttpHeaderCollection headers;
        HttpResponse response(HttpStatus::Ok(), std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("abc")), std::move(headers));

        auto firstBody = response.getBody();
        TEST_ASSERT_NOT_NULL(firstBody.get());
        TEST_ASSERT_EQUAL_STRING("abc", lumalink::http::TestSupport::ReadByteSourceAsStdString(*firstBody).c_str());

        auto secondBody = response.getBody();
        TEST_ASSERT_NULL(secondBody.get());
    }

    void test_http_response_accepts_empty_body_source()
    {
        HttpHeaderCollection headers;
        HttpResponse response(HttpStatus::Ok(), std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(), std::move(headers));

        auto body = response.getBody();
        TEST_ASSERT_NOT_NULL(body.get());
        TEST_ASSERT_EQUAL_STRING("", lumalink::http::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    void test_string_response_sets_default_headers_and_preserves_custom_content_type()
    {
        auto response = StringResponse::create(
            HttpStatus::Created(),
            "body",
            {
                HttpHeader::ContentType("application/custom"),
                HttpHeader::Connection("keep-alive")
            });

        TEST_ASSERT_EQUAL(201, static_cast<int>(response->status()));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentType, "application/custom"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentLength, "4"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::Connection, "keep-alive"));

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING("body", lumalink::http::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    void test_string_response_content_type_overload_keeps_exact_body_bytes()
    {
        const uint8_t bytes[] = {0x41, 0x00, 0x42};
        auto binaryResponse = StringResponse::create(HttpStatus::Ok(), bytes, sizeof(bytes));
        auto binaryBody = binaryResponse->getBody();
        const std::vector<uint8_t> binaryContent = ReadAsVector(*binaryBody);

        TEST_ASSERT_EQUAL_UINT64(sizeof(bytes), binaryContent.size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(bytes, binaryContent.data(), sizeof(bytes));

        auto typedResponse = StringResponse::create(HttpStatus::Accepted(), "text/csv", std::string("a,b"));
        TEST_ASSERT_TRUE(typedResponse->headers().exists(HttpHeaderNames::ContentType, "text/csv"));
        TEST_ASSERT_TRUE(typedResponse->headers().exists(HttpHeaderNames::ContentLength, "3"));

        auto typedBody = typedResponse->getBody();
        TEST_ASSERT_EQUAL_STRING("a,b", lumalink::http::TestSupport::ReadByteSourceAsStdString(*typedBody).c_str());
    }

    void test_form_response_sets_defaults_and_preserves_explicit_headers()
    {
        FormResponse::FieldCollection fields;
        fields.emplace_back("greeting", "hello world");
        fields.emplace_back("symbol", "1+1");

        auto response = FormResponse::create(
            HttpStatus::Ok(),
            fields,
            {
                HttpHeader::ContentType("application/custom-form"),
                HttpHeader::Connection("keep-alive")
            });

        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentType, "application/custom-form"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::Connection, "keep-alive"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentLength, "33"));

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING("greeting=hello+world&symbol=1%2B1", lumalink::http::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    void test_form_response_map_overload_preserves_key_order_from_map()
    {
        FormResponse::FieldMap fields;
        fields.emplace("beta", "2");
        fields.emplace("alpha", "1");

        auto response = FormResponse::create(HttpStatus::Ok(), fields);
        auto body = response->getBody();

        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentType, "application/x-www-form-urlencoded"));
        TEST_ASSERT_EQUAL_STRING("alpha=1&beta=2", lumalink::http::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
    void test_json_response_sets_default_headers_and_preserves_explicit_headers()
    {
        JsonDocument document;
        document["message"] = "ok";

        auto response = JsonResponse::create(
            HttpStatus::Accepted(),
            document,
            {
                HttpHeader::ContentType("application/merge-patch+json"),
                HttpHeader::Connection("keep-alive")
            });

        TEST_ASSERT_EQUAL(202, static_cast<int>(response->status()));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentType, "application/merge-patch+json"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentLength, "16"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::Connection, "keep-alive"));

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING("{\"message\":\"ok\"}", lumalink::http::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }
#endif

    void test_create_response_stream_serializes_direct_response_exactly()
    {
        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("Hi")),
            MakeDeterministicResponseHeaders());

        auto source = CreateResponseStream(std::move(response));
        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*source);

        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "Hi", content.c_str());
    }

    void test_create_response_stream_serializes_empty_response_without_body()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeader::Date("Thu, 01 Jan 1970 00:00:00 GMT"));
        headers.set(HttpHeader::Server("UnitTest"));
        headers.set(HttpHeader::ContentType("text/plain"));
        headers.set(HttpHeader::Connection("close"));

        auto response = std::make_unique<HttpResponse>(HttpStatus::Ok(), nullptr, std::move(headers));

        auto source = CreateResponseStream(std::move(response));
        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*source);

        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            content.c_str());
    }

    void test_create_response_stream_no_content_status_suppresses_body_but_keeps_length_header()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeader::Date("Thu, 01 Jan 1970 00:00:00 GMT"));
        headers.set(HttpHeader::Server("UnitTest"));
        headers.set(HttpHeader::ContentType("text/plain"));
        headers.set(HttpHeader::Connection("close"));

        auto response = std::make_unique<HttpResponse>(
            HttpStatus::NoContent(),
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("body")),
            std::move(headers));

        auto source = CreateResponseStream(std::move(response));
        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*source);

        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 204 No Content\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Content-Length: 4\r\n"
            "\r\n",
            content.c_str());
    }

    void test_create_response_stream_informational_and_not_modified_statuses_suppress_body()
    {
        {
            auto response = std::make_unique<HttpResponse>(
                HttpStatus::Continue(),
                std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("body")),
                MakeDeterministicResponseHeaders());

            auto source = CreateResponseStream(std::move(response));
            const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*source);

            TEST_ASSERT_EQUAL_STRING(
                "HTTP/1.1 100 Continue\r\n"
                "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
                "Server: UnitTest\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n"
                "Content-Length: 4\r\n"
                "\r\n",
                content.c_str());
        }

        {
            auto response = std::make_unique<HttpResponse>(
                HttpStatus::NotModified(),
                std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("body")),
                MakeDeterministicResponseHeaders());

            auto source = CreateResponseStream(std::move(response));
            const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*source);

            TEST_ASSERT_EQUAL_STRING(
                "HTTP/1.1 304 Not Modified\r\n"
                "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
                "Server: UnitTest\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n"
                "Content-Length: 4\r\n"
                "\r\n",
                content.c_str());
        }
    }

    void test_create_response_stream_preserves_explicit_transfer_encoding_with_null_body_source()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeader::Date("Thu, 01 Jan 1970 00:00:00 GMT"));
        headers.set(HttpHeader::Server("UnitTest"));
        headers.set(HttpHeader::ContentType("text/plain"));
        headers.set(HttpHeader::Connection("close"));
        headers.set(HttpHeader::TransferEncoding("chunked"));

        auto response = std::make_unique<HttpResponse>(HttpStatus::Ok(), nullptr, std::move(headers));

        auto source = CreateResponseStream(std::move(response));
        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*source);

        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            content.c_str());
    }

    void test_create_response_stream_preserves_explicit_content_length_and_transfer_encoding_conflict()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeader::Date("Thu, 01 Jan 1970 00:00:00 GMT"));
        headers.set(HttpHeader::Server("UnitTest"));
        headers.set(HttpHeader::ContentType("text/plain"));
        headers.set(HttpHeader::Connection("close"));
        headers.set(HttpHeader::ContentLength("999"));
        headers.set(HttpHeader::TransferEncoding("chunked"));

        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("Hi")),
            std::move(headers));

        auto source = CreateResponseStream(std::move(response));
        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*source);

        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Content-Length: 999\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "2\r\n"
            "Hi\r\n"
            "0\r\n\r\n",
            content.c_str());
    }

    void test_create_response_stream_preserves_header_insertion_order()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeader::Date("Thu, 01 Jan 1970 00:00:00 GMT"));
        headers.set(HttpHeader::Server("UnitTest"));
        headers.set(HttpHeader::ContentType("text/plain"));
        headers.set(HttpHeader("X-First", "1"));
        headers.set(HttpHeader("X-Second", "2"));
        headers.set(HttpHeader::Connection("close"));

        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("!")),
            std::move(headers));

        auto source = CreateResponseStream(std::move(response));
        const std::string content = lumalink::http::TestSupport::ReadByteSourceAsStdString(*source);

        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "X-First: 1\r\n"
            "X-Second: 2\r\n"
            "Connection: close\r\n"
            "Content-Length: 1\r\n"
            "\r\n"
            "!",
            content.c_str());
    }

    void test_create_response_stream_peek_and_small_reads_cross_header_body_boundary()
    {
        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("Hi")),
            MakeDeterministicResponseHeaders());

        auto source = CreateResponseStream(std::move(response));
        std::string content;
        std::uint8_t peeked = 0;
        std::uint8_t buffer[7] = {};

        TEST_ASSERT_EQUAL_UINT64(1, source->peek(lumalink::span<std::uint8_t>(&peeked, 1)));
        TEST_ASSERT_EQUAL('H', static_cast<char>(peeked));

        while (true)
        {
            const std::size_t bytesRead = source->read(lumalink::span<std::uint8_t>(buffer, sizeof(buffer)));
            if (bytesRead == 0)
            {
                break;
            }

            content.append(reinterpret_cast<const char *>(buffer), bytesRead);
        }

        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "Hi",
            content.c_str());
    }

    void test_create_response_stream_serializes_chunked_response_exactly()
    {
        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"", true}, {"Hi", false}}),
            MakeDeterministicResponseHeaders());

        auto source = CreateResponseStream(std::move(response));
        const std::string content = lumalink::http::TestSupport::DrainByteSourceWithAvailability(*source);

        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "2\r\n"
            "Hi\r\n"
            "0\r\n\r\n", content.c_str());
    }

    void test_chunked_response_stream_handles_temporary_unavailability_between_chunks()
    {
        auto body = ChunkedHttpResponseBodyStream::create(
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"Hi", false}, {"", true}, {"!", false}}));

        const std::string content = lumalink::http::TestSupport::DrainByteSourceWithAvailability(*body);
        TEST_ASSERT_EQUAL_STRING("2\r\nHi\r\n1\r\n!\r\n0\r\n\r\n", content.c_str());
    }

    void test_chunked_response_stream_handles_temporary_unavailability_across_multiple_boundaries()
    {
        auto body = ChunkedHttpResponseBodyStream::create(
            std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"AB", false}, {"", true}, {"CD", false}, {"", true}, {"E", false}}));

        const std::string content = lumalink::http::TestSupport::DrainByteSourceWithAvailability(*body);
        TEST_ASSERT_EQUAL_STRING("2\r\nAB\r\n2\r\nCD\r\n1\r\nE\r\n0\r\n\r\n", content.c_str());
    }

    void test_recording_byte_channel_captures_written_bytes_and_flushes()
    {
        lumalink::http::TestSupport::RecordingByteChannel channel;

        const std::string payload = "hello";
        const std::size_t bytesWritten = channel.write(std::string_view(payload));
        channel.flush();

        TEST_ASSERT_EQUAL_UINT64(payload.size(), bytesWritten);
        TEST_ASSERT_EQUAL_STRING("hello", channel.writtenText().c_str());
        TEST_ASSERT_EQUAL_UINT64(1, channel.flushCount());
        TEST_ASSERT_EQUAL_UINT64(1, channel.writeSizes().size());
        TEST_ASSERT_EQUAL_UINT64(payload.size(), channel.writeSizes()[0]);
    }

    void test_recording_byte_channel_can_expose_scripted_input()
    {
        lumalink::http::TestSupport::RecordingByteChannel channel({{"A", false}, {"", true}, {"BC", false}});

        const std::string content = lumalink::http::TestSupport::DrainByteSourceWithAvailability(channel);

        TEST_ASSERT_EQUAL_STRING("ABC", content.c_str());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_byte_source_reads_direct_body_content);
        RUN_TEST(test_byte_source_reports_temporarily_unavailable);
        RUN_TEST(test_chunked_response_body_stream_reads_from_byte_source);
        RUN_TEST(test_chunked_response_body_stream_emits_final_terminator_for_empty_source);
        RUN_TEST(test_chunked_response_body_stream_preserves_varying_source_chunk_sizes);
        RUN_TEST(test_chunked_response_body_stream_splits_large_available_span_at_frame_boundary);
        RUN_TEST(test_http_response_accepts_byte_source_body);
        RUN_TEST(test_http_response_returns_status_and_mutable_headers);
        RUN_TEST(test_http_response_transfers_body_ownership_once);
        RUN_TEST(test_http_response_accepts_empty_body_source);
        RUN_TEST(test_string_response_sets_default_headers_and_preserves_custom_content_type);
        RUN_TEST(test_string_response_content_type_overload_keeps_exact_body_bytes);
        RUN_TEST(test_form_response_sets_defaults_and_preserves_explicit_headers);
        RUN_TEST(test_form_response_map_overload_preserves_key_order_from_map);
    #if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
        RUN_TEST(test_json_response_sets_default_headers_and_preserves_explicit_headers);
    #endif
        RUN_TEST(test_create_response_stream_serializes_direct_response_exactly);
        RUN_TEST(test_create_response_stream_serializes_empty_response_without_body);
        RUN_TEST(test_create_response_stream_no_content_status_suppresses_body_but_keeps_length_header);
        RUN_TEST(test_create_response_stream_informational_and_not_modified_statuses_suppress_body);
        RUN_TEST(test_create_response_stream_preserves_explicit_transfer_encoding_with_null_body_source);
        RUN_TEST(test_create_response_stream_preserves_explicit_content_length_and_transfer_encoding_conflict);
        RUN_TEST(test_create_response_stream_preserves_header_insertion_order);
        RUN_TEST(test_create_response_stream_peek_and_small_reads_cross_header_body_boundary);
        RUN_TEST(test_create_response_stream_serializes_chunked_response_exactly);
        RUN_TEST(test_chunked_response_stream_handles_temporary_unavailability_between_chunks);
        RUN_TEST(test_chunked_response_stream_handles_temporary_unavailability_across_multiple_boundaries);
        RUN_TEST(test_recording_byte_channel_captures_written_bytes_and_flushes);
        RUN_TEST(test_recording_byte_channel_can_expose_scripted_input);
        return UNITY_END();
    }
}

int run_test_response_streams()
{
    return lumalink::http::TestSupport::RunConsolidatedSuite(
        "response streams",
        runUnitySuite,
        localSetUp,
        localTearDown);
}
