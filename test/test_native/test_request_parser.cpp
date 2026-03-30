#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include "../../src/httpadv/v1/HttpServerAdvanced.h"

#include <unity.h>

#include "../../src/httpadv/v1/core/Defines.h"
#include "../../src/httpadv/v1/pipeline/RequestParser.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using namespace httpadv::v1::core;
using namespace httpadv::v1::handlers;
using namespace httpadv::v1::pipeline;
using namespace httpadv::v1::response;
using namespace httpadv::v1::routing;
using namespace httpadv::v1::server;
using namespace httpadv::v1::staticfiles;
using namespace httpadv::v1::transport;
using namespace httpadv::v1::util;
using namespace httpadv::v1::websocket;

namespace
{
    constexpr std::string_view MaxSupportedCustomMethod = "UNSUBSCRIBE";
    static_assert(MaxSupportedCustomMethod.size() == httpadv::v1::core::MAX_REQUEST_METHOD_LENGTH);

    class CallbackReturnRecorder : public httpadv::v1::TestSupport::PipelineEventRecorder
    {
    public:
        int onHeadersComplete() override
        {
            const int baseResult = httpadv::v1::TestSupport::PipelineEventRecorder::onHeadersComplete();
            (void)baseResult;
            return headersCompleteReturnCode;
        }

        int onBody(const std::uint8_t *at, std::size_t length) override
        {
            const int baseResult = httpadv::v1::TestSupport::PipelineEventRecorder::onBody(at, length);
            (void)baseResult;
            return bodyReturnCode;
        }

        int onMessageComplete() override
        {
            const int baseResult = httpadv::v1::TestSupport::PipelineEventRecorder::onMessageComplete();
            (void)baseResult;
            return messageCompleteReturnCode;
        }

        int headersCompleteReturnCode = 0;
        int bodyReturnCode = 0;
        int messageCompleteReturnCode = 0;
    };

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

    std::string BuildRequestWithHeaders(
        std::string_view requestLine,
        const std::vector<std::pair<std::string, std::string>> &headers,
        std::string_view body = {})
    {
        std::string request;
        request.append(requestLine);
        request.append("\r\n");
        for (const auto &header : headers)
        {
            request.append(header.first);
            request.append(": ");
            request.append(header.second);
            request.append("\r\n");
        }
        request.append("\r\n");
        request.append(body);
        return request;
    }

    std::string RepeatChar(char value, std::size_t count)
    {
        return std::string(count, value);
    }

    std::size_t ExecuteText(RequestParser &parser, std::string_view text)
    {
        return parser.execute(reinterpret_cast<const std::uint8_t *>(text.data()), text.size());
    }

    std::size_t FirstEventIndex(const std::vector<httpadv::v1::TestSupport::RecordedPipelineEvent> &events, httpadv::v1::TestSupport::PipelineEventKind kind)
    {
        for (std::size_t i = 0; i < events.size(); ++i)
        {
            if (events[i].kind == kind)
            {
                return i;
            }
        }

        return events.size();
    }

    void test_request_parser_accepts_custom_method_verbatim()
    {
        httpadv::v1::TestSupport::PipelineEventRecorder recorder;
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
        TEST_ASSERT_TRUE(recorder.events()[0].kind == httpadv::v1::TestSupport::PipelineEventKind::MessageBegin);
    }

    void test_request_parser_preserves_split_custom_method_bytes()
    {
        httpadv::v1::TestSupport::PipelineEventRecorder recorder;
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
        httpadv::v1::TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string request = BuildRequest("BAD(METHOD", "/broken");
        const std::size_t consumed = ExecuteText(parser, request);

        TEST_ASSERT_EQUAL_UINT64(0, consumed);
        TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
        TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::InvalidMethodError);
        TEST_ASSERT_TRUE(recorder.events()[0].kind == httpadv::v1::TestSupport::PipelineEventKind::Error);
    }

    void test_request_parser_accepts_custom_method_at_explicit_limit()
    {
        httpadv::v1::TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string request = BuildRequest(MaxSupportedCustomMethod, "/limit");
        const std::size_t consumed = ExecuteText(parser, request);

        TEST_ASSERT_EQUAL_UINT64(request.size(), consumed);
        TEST_ASSERT_EQUAL_STRING(MaxSupportedCustomMethod.data(), recorder.method().c_str());
        TEST_ASSERT_TRUE(recorder.errors().empty());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.messageCompleteCount());
    }

    void test_request_parser_rejects_custom_method_past_explicit_limit()
    {
        httpadv::v1::TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string method = std::string(httpadv::v1::core::MAX_REQUEST_METHOD_LENGTH + 1U, 'M');
        const std::string request = BuildRequest(method, "/too-long");
        const std::size_t consumed = ExecuteText(parser, request);

        TEST_ASSERT_EQUAL_UINT64(0, consumed);
        TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
        TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::InvalidMethodError);
        TEST_ASSERT_TRUE(recorder.events()[0].kind == httpadv::v1::TestSupport::PipelineEventKind::Error);
    }

    void test_request_parser_accepts_uri_exactly_at_limit_and_rejects_uri_past_limit()
    {
        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            const std::string uri = "/" + RepeatChar('u', httpadv::v1::core::MAX_REQUEST_URI_LENGTH - 1);
            const std::string request = BuildRequest("GET", uri);
            const std::size_t consumed = ExecuteText(parser, request);

            TEST_ASSERT_EQUAL_UINT64(request.size(), consumed);
            TEST_ASSERT_EQUAL_STRING(uri.c_str(), recorder.url().c_str());
            TEST_ASSERT_TRUE(recorder.errors().empty());
        }

        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            const std::string uri = "/" + RepeatChar('u', httpadv::v1::core::MAX_REQUEST_URI_LENGTH);
            const std::string request = BuildRequest("GET", uri);
            const std::size_t consumed = ExecuteText(parser, request);

            TEST_ASSERT_EQUAL_UINT64(0, consumed);
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::UriTooLong);
        }
    }

    void test_request_parser_accepts_header_name_and_value_at_limits_and_rejects_overflow()
    {
        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            const std::string headerName = RepeatChar('N', httpadv::v1::core::MAX_REQUEST_HEADER_NAME_LENGTH);
            const std::string headerValue = RepeatChar('V', httpadv::v1::core::MAX_REQUEST_HEADER_VALUE_LENGTH);
            const std::string request = BuildRequestWithHeaders(
                "GET /limit HTTP/1.1",
                {
                    {headerName, headerValue}
                });
            const std::size_t consumed = ExecuteText(parser, request);

            TEST_ASSERT_EQUAL_UINT64(request.size(), consumed);
            TEST_ASSERT_EQUAL_UINT64(1, recorder.headers().size());
            TEST_ASSERT_EQUAL_STRING(headerName.c_str(), recorder.headers()[0].first.c_str());
            TEST_ASSERT_EQUAL_STRING(headerValue.c_str(), recorder.headers()[0].second.c_str());
            TEST_ASSERT_TRUE(recorder.errors().empty());
        }

        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            const std::string tooLongHeaderName = RepeatChar('N', httpadv::v1::core::MAX_REQUEST_HEADER_NAME_LENGTH + 1);
            const std::string request = BuildRequestWithHeaders(
                "GET /overflow-name HTTP/1.1",
                {
                    {tooLongHeaderName, "ok"}
                });
            const std::size_t consumed = ExecuteText(parser, request);

            TEST_ASSERT_EQUAL_UINT64(0, consumed);
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::HeaderTooLarge);
        }

        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            const std::string tooLongHeaderValue = RepeatChar('V', httpadv::v1::core::MAX_REQUEST_HEADER_VALUE_LENGTH + 1);
            const std::string request = BuildRequestWithHeaders(
                "GET /overflow-value HTTP/1.1",
                {
                    {"X-Name", tooLongHeaderValue}
                });
            const std::size_t consumed = ExecuteText(parser, request);

            TEST_ASSERT_EQUAL_UINT64(0, consumed);
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::HeaderTooLarge);
        }
    }

    void test_request_parser_accepts_header_count_at_limit_and_rejects_overflow_header_count()
    {
        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            std::vector<std::pair<std::string, std::string>> headers;
            headers.reserve(httpadv::v1::core::MAX_REQUEST_HEADER_COUNT);
            for (std::size_t i = 0; i < httpadv::v1::core::MAX_REQUEST_HEADER_COUNT; ++i)
            {
                headers.emplace_back("X-H" + std::to_string(i), "v");
            }

            const std::string request = BuildRequestWithHeaders("GET /header-limit HTTP/1.1", headers);
            const std::size_t consumed = ExecuteText(parser, request);

            TEST_ASSERT_EQUAL_UINT64(request.size(), consumed);
            TEST_ASSERT_EQUAL_UINT64(httpadv::v1::core::MAX_REQUEST_HEADER_COUNT, recorder.headers().size());
            TEST_ASSERT_TRUE(recorder.errors().empty());
        }

        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            std::vector<std::pair<std::string, std::string>> headers;
            headers.reserve(httpadv::v1::core::MAX_REQUEST_HEADER_COUNT + 1);
            for (std::size_t i = 0; i < httpadv::v1::core::MAX_REQUEST_HEADER_COUNT + 1; ++i)
            {
                headers.emplace_back("X-O" + std::to_string(i), "v");
            }

            const std::string request = BuildRequestWithHeaders("GET /header-overflow HTTP/1.1", headers);
            const std::size_t consumed = ExecuteText(parser, request);

            TEST_ASSERT_EQUAL_UINT64(0, consumed);
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::HeaderTooLarge);
        }
    }

    void test_request_parser_maps_malformed_request_errors_for_url_header_token_content_length_and_version()
    {
        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);
            const std::string request = "GET /bad url HTTP/1.1\r\nHost: example.test\r\n\r\n";

            TEST_ASSERT_EQUAL_UINT64(0, ExecuteText(parser, request));
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(
                recorder.errors()[0] == PipelineErrorCode::InvalidUrlError ||
                recorder.errors()[0] == PipelineErrorCode::InvalidConstantError ||
                recorder.errors()[0] == PipelineErrorCode::ParseError);
        }

        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);
            const std::string request = "GET /ok HTTP/1.1\r\nBad[Header: value\r\n\r\n";

            TEST_ASSERT_EQUAL_UINT64(0, ExecuteText(parser, request));
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::InvalidHeaderTokenError);
        }

        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);
            const std::string request = "GET /ok HTTP/1.1\r\nContent-Length: nope\r\n\r\n";

            TEST_ASSERT_EQUAL_UINT64(0, ExecuteText(parser, request));
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::InvalidContentLengthError);
        }

        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);
            const std::string request = "GET /ok HTTP/12.1\r\nHost: example.test\r\n\r\n";

            TEST_ASSERT_EQUAL_UINT64(0, ExecuteText(parser, request));
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] != PipelineErrorCode::None);
        }
    }

    void test_request_parser_orders_events_and_stitches_split_headers_and_body_chunks()
    {
        httpadv::v1::TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string chunk1 = "POST";
        const std::string chunk2 = " /order HTTP/1.1\r\n";
        const std::string chunk3 = "Host: e";
        const std::string chunk4 = "xample.test\r\nX-Nam";
        const std::string chunk5 = "e: a";
        const std::string chunk6 = "bc\r\nContent-Length: 5\r\n\r\nhe";
        const std::string chunk7 = "llo";

        TEST_ASSERT_EQUAL_UINT64(chunk1.size(), ExecuteText(parser, chunk1));
        TEST_ASSERT_EQUAL_UINT64(chunk2.size(), ExecuteText(parser, chunk2));
        TEST_ASSERT_EQUAL_UINT64(chunk3.size(), ExecuteText(parser, chunk3));
        TEST_ASSERT_EQUAL_UINT64(chunk4.size(), ExecuteText(parser, chunk4));
        TEST_ASSERT_EQUAL_UINT64(chunk5.size(), ExecuteText(parser, chunk5));
        TEST_ASSERT_EQUAL_UINT64(chunk6.size(), ExecuteText(parser, chunk6));
        TEST_ASSERT_EQUAL_UINT64(chunk7.size(), ExecuteText(parser, chunk7));

        TEST_ASSERT_EQUAL_STRING("POST", recorder.method().c_str());
        TEST_ASSERT_EQUAL_STRING("/order", recorder.url().c_str());
        TEST_ASSERT_EQUAL_UINT64(3, recorder.headers().size());
        TEST_ASSERT_EQUAL_STRING("Host", recorder.headers()[0].first.c_str());
        TEST_ASSERT_EQUAL_STRING("example.test", recorder.headers()[0].second.c_str());
        TEST_ASSERT_EQUAL_STRING("X-Name", recorder.headers()[1].first.c_str());
        TEST_ASSERT_EQUAL_STRING("abc", recorder.headers()[1].second.c_str());
        TEST_ASSERT_EQUAL_STRING("hello", recorder.bodyText().c_str());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.headersCompleteCount());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.messageCompleteCount());
        TEST_ASSERT_TRUE(recorder.errors().empty());

        const auto &events = recorder.events();
        const std::size_t messageBeginIndex = FirstEventIndex(events, httpadv::v1::TestSupport::PipelineEventKind::MessageBegin);
        const std::size_t headersCompleteIndex = FirstEventIndex(events, httpadv::v1::TestSupport::PipelineEventKind::HeadersComplete);
        const std::size_t bodyIndex = FirstEventIndex(events, httpadv::v1::TestSupport::PipelineEventKind::BodyChunk);
        const std::size_t messageCompleteIndex = FirstEventIndex(events, httpadv::v1::TestSupport::PipelineEventKind::MessageComplete);

        TEST_ASSERT_TRUE(messageBeginIndex < headersCompleteIndex);
        TEST_ASSERT_TRUE(headersCompleteIndex < bodyIndex);
        TEST_ASSERT_TRUE(bodyIndex < messageCompleteIndex);
    }

    void test_request_parser_propagates_non_zero_return_codes_from_callback_points_as_parse_error()
    {
        {
            CallbackReturnRecorder recorder;
            recorder.headersCompleteReturnCode = -1;
            RequestParser parser(recorder);
            const std::string request = "GET /headers-complete HTTP/1.1\r\nHost: example.test\r\n\r\n";

            TEST_ASSERT_EQUAL_UINT64(0, ExecuteText(parser, request));
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::ParseError);
        }

        {
            CallbackReturnRecorder recorder;
            recorder.bodyReturnCode = -2;
            RequestParser parser(recorder);
            const std::string request = "POST /body HTTP/1.1\r\nHost: example.test\r\nContent-Length: 3\r\n\r\nabc";

            TEST_ASSERT_EQUAL_UINT64(0, ExecuteText(parser, request));
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::ParseError);
        }

        {
            CallbackReturnRecorder recorder;
            recorder.messageCompleteReturnCode = -3;
            RequestParser parser(recorder);
            const std::string request = "GET /complete HTTP/1.1\r\nHost: example.test\r\n\r\n";

            TEST_ASSERT_EQUAL_UINT64(0, ExecuteText(parser, request));
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::ParseError);
        }
    }

    void test_request_parser_reset_allows_reuse_for_subsequent_request()
    {
        httpadv::v1::TestSupport::PipelineEventRecorder recorder;
        RequestParser parser(recorder);

        const std::string firstRequest = BuildRequest("GET", "/first");
        TEST_ASSERT_EQUAL_UINT64(firstRequest.size(), ExecuteText(parser, firstRequest));
        TEST_ASSERT_EQUAL_UINT64(1, recorder.messageCompleteCount());
        TEST_ASSERT_EQUAL_STRING("/first", recorder.url().c_str());

        parser.reset();

        const std::string secondRequest = BuildRequest("POST", "/second");
        TEST_ASSERT_EQUAL_UINT64(secondRequest.size(), ExecuteText(parser, secondRequest));
        TEST_ASSERT_EQUAL_UINT64(2, recorder.messageCompleteCount());
        TEST_ASSERT_EQUAL_STRING("POST", recorder.method().c_str());
        TEST_ASSERT_EQUAL_STRING("/second", recorder.url().c_str());
        TEST_ASSERT_TRUE(recorder.errors().empty());
    }

    void test_request_parser_end_of_input_finishes_complete_messages_and_errors_on_incomplete_input()
    {
        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            const std::string request = BuildRequest("GET", "/eof-ok");
            TEST_ASSERT_EQUAL_UINT64(request.size(), ExecuteText(parser, request));
            TEST_ASSERT_EQUAL_UINT64(0, parser.execute(nullptr, 0));
            TEST_ASSERT_TRUE(recorder.errors().empty());
            TEST_ASSERT_EQUAL_UINT64(1, recorder.messageCompleteCount());
        }

        {
            httpadv::v1::TestSupport::PipelineEventRecorder recorder;
            RequestParser parser(recorder);

            const std::string incomplete = "GET /eof-fail HTTP/1.1\r\n";
            TEST_ASSERT_EQUAL_UINT64(incomplete.size(), ExecuteText(parser, incomplete));
            TEST_ASSERT_EQUAL_UINT64(0, parser.execute(nullptr, 0));
            TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
            TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::InvalidEofStateError);
        }
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_request_parser_accepts_custom_method_verbatim);
        RUN_TEST(test_request_parser_preserves_split_custom_method_bytes);
        RUN_TEST(test_request_parser_rejects_invalid_custom_method_token);
        RUN_TEST(test_request_parser_accepts_custom_method_at_explicit_limit);
        RUN_TEST(test_request_parser_rejects_custom_method_past_explicit_limit);
        RUN_TEST(test_request_parser_accepts_uri_exactly_at_limit_and_rejects_uri_past_limit);
        RUN_TEST(test_request_parser_accepts_header_name_and_value_at_limits_and_rejects_overflow);
        RUN_TEST(test_request_parser_accepts_header_count_at_limit_and_rejects_overflow_header_count);
        RUN_TEST(test_request_parser_maps_malformed_request_errors_for_url_header_token_content_length_and_version);
        RUN_TEST(test_request_parser_orders_events_and_stitches_split_headers_and_body_chunks);
        RUN_TEST(test_request_parser_propagates_non_zero_return_codes_from_callback_points_as_parse_error);
        RUN_TEST(test_request_parser_reset_allows_reuse_for_subsequent_request);
        RUN_TEST(test_request_parser_end_of_input_finishes_complete_messages_and_errors_on_incomplete_input);
        return UNITY_END();
    }
}

int run_test_request_parser()
{
    return httpadv::v1::TestSupport::RunConsolidatedSuite(
        "request parser",
        runUnitySuite,
        localSetUp,
        localTearDown);
}