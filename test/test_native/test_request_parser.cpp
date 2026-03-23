#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/pipeline/RequestParser.h"

#include <cstdint>
#include <string>
#include <string_view>

using namespace HttpServerAdvanced;

namespace
{
    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    std::string BuildRequest(std::string_view method, std::string_view url = "/resource")
    {
        std::string request;
        request.reserve(method.size() + url.size() + 32);
        request.append(method);
        request.append(" ");
        request.append(url);
        request.append(" HTTP/1.1\r\nHost: example.test\r\n\r\n");
        return request;
    }

    std::size_t ExecuteText(RequestParser &parser, std::string_view text)
    {
        return parser.execute(reinterpret_cast<const std::uint8_t *>(text.data()), text.size());
    }

    void test_request_parser_accepts_custom_method_verbatim()
    {
        TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string request = BuildRequest("PURGE", "/cache");
        const std::size_t consumed = ExecuteText(parser, request);

        TEST_ASSERT_EQUAL_UINT64(request.size(), consumed);
        TEST_ASSERT_EQUAL_STRING("PURGE", recorder.method().c_str());
        TEST_ASSERT_EQUAL_STRING("/cache", recorder.url().c_str());
        TEST_ASSERT_EQUAL_UINT16(1, recorder.versionMajor());
        TEST_ASSERT_EQUAL_UINT16(1, recorder.versionMinor());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.headers().size());
        TEST_ASSERT_EQUAL_STRING("Host", recorder.headers()[0].first.c_str());
        TEST_ASSERT_EQUAL_STRING("example.test", recorder.headers()[0].second.c_str());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.headersCompleteCount());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.messageCompleteCount());
        TEST_ASSERT_TRUE(recorder.errors().empty());
        TEST_ASSERT_TRUE(parser.shouldKeepAlive());
        TEST_ASSERT_TRUE(recorder.events()[0].kind == TestSupport::PipelineEventKind::MessageBegin);
    }

    void test_request_parser_preserves_split_custom_method_bytes()
    {
        TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string secondChunk = "SEARCH /upnp HTTP/1.1\r\nHost: example.test\r\n\r\n";

        TEST_ASSERT_EQUAL_UINT64(2, ExecuteText(parser, "M-"));
        TEST_ASSERT_TRUE(recorder.events().empty());

        const std::size_t consumed = ExecuteText(parser, secondChunk);

        TEST_ASSERT_EQUAL_UINT64(secondChunk.size(), consumed);
        TEST_ASSERT_EQUAL_STRING("M-SEARCH", recorder.method().c_str());
        TEST_ASSERT_EQUAL_STRING("/upnp", recorder.url().c_str());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.messageCompleteCount());
        TEST_ASSERT_TRUE(recorder.errors().empty());
    }

    void test_request_parser_rejects_invalid_custom_method_token()
    {
        TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string request = BuildRequest("BAD(METHOD", "/broken");
        const std::size_t consumed = ExecuteText(parser, request);

        TEST_ASSERT_EQUAL_UINT64(0, consumed);
        TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
        TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::InvalidMethodError);
        TEST_ASSERT_TRUE(recorder.events()[0].kind == TestSupport::PipelineEventKind::Error);
    }

    void test_request_parser_rejects_sixteen_character_custom_method_at_current_boundary()
    {
        TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string request = BuildRequest("ABCDEFGHIJKLMNOP", "/too-long");
        const std::size_t consumed = ExecuteText(parser, request);

        TEST_ASSERT_EQUAL_UINT64(0, consumed);
        TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
        TEST_ASSERT_TRUE(recorder.errors()[0] != PipelineErrorCode::None);
        TEST_ASSERT_TRUE(recorder.events()[0].kind == TestSupport::PipelineEventKind::Error);
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_request_parser_accepts_custom_method_verbatim);
        RUN_TEST(test_request_parser_preserves_split_custom_method_bytes);
        RUN_TEST(test_request_parser_rejects_invalid_custom_method_token);
        RUN_TEST(test_request_parser_rejects_sixteen_character_custom_method_at_current_boundary);
        return UNITY_END();
    }
}

int run_test_request_parser()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "request parser",
        runUnitySuite,
        localSetUp,
        localTearDown);
}