#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/ByteStreamFixtures.h"

#include <unity.h>

#include "../../src/response/ChunkedHttpResponseBodyStream.h"
#include "../../src/response/HttpResponse.h"
#include "../../src/streams/ByteStream.h"
#include "../../src/streams/Streams.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace HttpServerAdvanced;

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
        auto body = std::make_unique<TestSupport::ScriptedByteSource>(TestSupport::ScriptedByteSource::FromText("ok"));

        const std::string content = TestSupport::ReadByteSourceAsStdString(*body);
        TEST_ASSERT_EQUAL_STRING("ok", content.c_str());
    }

    void test_byte_source_reports_temporarily_unavailable()
    {
        auto body = std::make_unique<TestSupport::ScriptedByteSource>(
            std::initializer_list<std::pair<const char *, bool>>{{"", true}});

        TEST_ASSERT_TRUE(body->available().isTemporarilyUnavailable());
    }

    void test_chunked_response_body_stream_reads_from_byte_source()
    {
        auto body = ChunkedHttpResponseBodyStream::create(
            std::make_unique<TestSupport::ScriptedByteSource>(TestSupport::ScriptedByteSource::FromText("Hi")));

        const std::string content = TestSupport::ReadByteSourceAsStdString(*body);
        TEST_ASSERT_EQUAL_STRING("2\r\nHi\r\n0\r\n\r\n", content.c_str());
    }

    void test_http_response_accepts_byte_source_body()
    {
        HttpHeaderCollection headers;
        HttpResponse response(HttpStatus::Ok(), std::make_unique<TestSupport::ScriptedByteSource>(TestSupport::ScriptedByteSource::FromText("body")), std::move(headers));

        auto body = response.getBody();
        const std::string content = TestSupport::ReadByteSourceAsStdString(*body);
        TEST_ASSERT_EQUAL_STRING("body", content.c_str());
    }

    void test_create_response_stream_serializes_direct_response_exactly()
    {
        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<TestSupport::ScriptedByteSource>(TestSupport::ScriptedByteSource::FromText("Hi")),
            MakeDeterministicResponseHeaders());

        auto source = CreateResponseStream(std::move(response));
        const std::string content = TestSupport::ReadByteSourceAsStdString(*source);

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

    void test_create_response_stream_serializes_chunked_response_exactly()
    {
        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<TestSupport::ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"", true}, {"Hi", false}}),
            MakeDeterministicResponseHeaders());

        auto source = CreateResponseStream(std::move(response));
        const std::string content = TestSupport::DrainByteSourceWithAvailability(*source);

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
            std::make_unique<TestSupport::ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"Hi", false}, {"", true}, {"!", false}}));

        const std::string content = TestSupport::DrainByteSourceWithAvailability(*body);
        TEST_ASSERT_EQUAL_STRING("2\r\nHi\r\n1\r\n!\r\n0\r\n\r\n", content.c_str());
    }

    void test_recording_byte_channel_captures_written_bytes_and_flushes()
    {
        TestSupport::RecordingByteChannel channel;

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
        TestSupport::RecordingByteChannel channel({{"A", false}, {"", true}, {"BC", false}});

        const std::string content = TestSupport::DrainByteSourceWithAvailability(channel);

        TEST_ASSERT_EQUAL_STRING("ABC", content.c_str());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_byte_source_reads_direct_body_content);
        RUN_TEST(test_byte_source_reports_temporarily_unavailable);
        RUN_TEST(test_chunked_response_body_stream_reads_from_byte_source);
        RUN_TEST(test_http_response_accepts_byte_source_body);
        RUN_TEST(test_create_response_stream_serializes_direct_response_exactly);
        RUN_TEST(test_create_response_stream_serializes_chunked_response_exactly);
        RUN_TEST(test_chunked_response_stream_handles_temporary_unavailability_between_chunks);
        RUN_TEST(test_recording_byte_channel_captures_written_bytes_and_flushes);
        RUN_TEST(test_recording_byte_channel_can_expose_scripted_input);
        return UNITY_END();
    }
}

int run_test_response_streams()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "response streams",
        runUnitySuite,
        localSetUp,
        localTearDown);
}