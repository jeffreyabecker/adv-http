#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/core/HttpRequest.h"
#include "../../src/core/HttpRequestPhase.h"
#include "../../src/handlers/BufferingHttpHandlerBase.h"
#include "../../src/server/HttpServerBase.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    template <ssize_t MaxBuffered>
    class BufferingProbeHandler : public BufferingHttpHandlerBase<MaxBuffered>
    {
    public:
        typename IHttpHandler::HandlerResult handleBody(HttpRequest &context, std::vector<uint8_t> &&body) override
        {
            phases.push_back(context.completedPhases());
            payloads.emplace_back(body.begin(), body.end());
            return nullptr;
        }

        std::vector<HttpRequestPhaseFlags> phases;
        std::vector<std::vector<uint8_t>> payloads;
    };

    class RequestHarness
    {
    public:
        explicit RequestHarness(std::unique_ptr<IHttpHandler> handler)
            : server_(std::make_unique<TestSupport::FakeServer>()),
              pendingHandler_(std::move(handler)),
              handlerPtr_(pendingHandler_.get()),
              factory_([this](HttpRequest &) -> std::unique_ptr<IHttpHandler>
              {
                  return std::move(pendingHandler_);
              }),
              pipeline_(HttpRequest::createPipelineHandler(server_, factory_))
        {
            pipeline_->setResponseStreamCallback([this](std::unique_ptr<IByteSource> responseStream)
            {
                ++responseStreamCount_;
                if (responseStream)
                {
                    responseText_ = TestSupport::ReadByteSourceAsStdString(*responseStream);
                }
            });
        }

        template <typename HandlerT>
        HandlerT *handlerAs() const
        {
            return static_cast<HandlerT *>(handlerPtr_);
        }

        void beginRequest(const char *method = "POST", const char *url = "/body")
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onMessageBegin(method, 1, 1, url));
        }

        void addHeader(std::string_view name, std::string_view value)
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeader(name, value));
        }

        void completeHeaders()
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeadersComplete());
        }

        void addBody(std::string_view text)
        {
            TEST_ASSERT_EQUAL_INT(
                0,
                pipeline_->onBody(
                    reinterpret_cast<const std::uint8_t *>(text.data()),
                    text.size()));
        }

        void completeMessage()
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onMessageComplete());
        }

        std::size_t responseStreamCount() const
        {
            return responseStreamCount_;
        }

        const std::string &responseText() const
        {
            return responseText_;
        }

    private:
        HttpServerBase server_;
        std::unique_ptr<IHttpHandler> pendingHandler_;
        IHttpHandler *handlerPtr_ = nullptr;
        TestSupport::RecordingRequestHandlerFactory factory_;
        std::unique_ptr<IPipelineHandler, std::function<void(IPipelineHandler *)>> pipeline_;
        std::size_t responseStreamCount_ = 0;
        std::string responseText_;
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_try_parse_header_length_accepts_complete_numeric_values()
    {
        size_t parsedLength = 999;

        TEST_ASSERT_TRUE(tryParseHeaderLength("0", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(0, parsedLength);

        TEST_ASSERT_TRUE(tryParseHeaderLength("12345", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(12345, parsedLength);
    }

    void test_try_parse_header_length_rejects_invalid_or_partial_values()
    {
        size_t parsedLength = 77;

        TEST_ASSERT_FALSE(tryParseHeaderLength("", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(0, parsedLength);

        TEST_ASSERT_FALSE(tryParseHeaderLength("12x", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(12, parsedLength);

        TEST_ASSERT_FALSE(tryParseHeaderLength(" 12", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(0, parsedLength);

        TEST_ASSERT_FALSE(tryParseHeaderLength("18446744073709551616", parsedLength));
    }

    void test_buffering_handler_does_not_buffer_when_disabled()
    {
        auto handler = std::make_unique<BufferingProbeHandler<0>>();
        BufferingProbeHandler<0> *handlerPtr = handler.get();
        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "4");
        harness.completeHeaders();
        harness.addBody("test");
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(0, harness.responseStreamCount());
    }

    void test_buffering_handler_reinvokes_handle_body_on_completion_after_content_length_is_reached()
    {
        auto handler = std::make_unique<BufferingProbeHandler<8>>();
        BufferingProbeHandler<8> *handlerPtr = handler.get();
        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "4");
        harness.completeHeaders();
        harness.addBody("test");

        TEST_ASSERT_EQUAL_UINT64(1, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(4, handlerPtr->payloads[0].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("test"), handlerPtr->payloads[0].data(), 4);
        TEST_ASSERT_TRUE((handlerPtr->phases[0] & HttpRequestPhase::BeginReadingBody) != 0);
        TEST_ASSERT_FALSE((handlerPtr->phases[0] & HttpRequestPhase::CompletedReadingMessage) != 0);

        harness.completeMessage();
        TEST_ASSERT_EQUAL_UINT64(2, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(4, handlerPtr->payloads[1].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("test"), handlerPtr->payloads[1].data(), 4);
        TEST_ASSERT_TRUE((handlerPtr->phases[1] & HttpRequestPhase::CompletedReadingMessage) != 0);
    }

    void test_buffering_handler_invokes_handle_body_from_handle_step_without_content_length()
    {
        auto handler = std::make_unique<BufferingProbeHandler<-1>>();
        BufferingProbeHandler<-1> *handlerPtr = handler.get();
        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.completeHeaders();
        harness.addBody("abc");
        harness.addBody("def");

        TEST_ASSERT_EQUAL_UINT64(0, handlerPtr->payloads.size());

        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(6, handlerPtr->payloads[0].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("abcdef"), handlerPtr->payloads[0].data(), 6);
        TEST_ASSERT_TRUE((handlerPtr->phases[0] & HttpRequestPhase::CompletedReadingMessage) != 0);
    }

    void test_buffering_handler_truncates_to_configured_max_buffer_and_replays_on_completion()
    {
        auto handler = std::make_unique<BufferingProbeHandler<4>>();
        BufferingProbeHandler<4> *handlerPtr = handler.get();
        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "8");
        harness.completeHeaders();
        harness.addBody("abcdef");
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(2, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(4, handlerPtr->payloads[0].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("abcd"), handlerPtr->payloads[0].data(), 4);
        TEST_ASSERT_EQUAL_UINT64(4, handlerPtr->payloads[1].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("abcd"), handlerPtr->payloads[1].data(), 4);
    }

    void test_buffering_handler_ignores_empty_body_and_zero_content_length()
    {
        auto missingLengthHandler = std::make_unique<BufferingProbeHandler<16>>();
        BufferingProbeHandler<16> *missingLengthPtr = missingLengthHandler.get();
        RequestHarness missingLengthHarness(std::move(missingLengthHandler));

        missingLengthHarness.beginRequest();
        missingLengthHarness.completeHeaders();
        missingLengthHarness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, missingLengthPtr->payloads.size());

        auto zeroLengthHandler = std::make_unique<BufferingProbeHandler<16>>();
        BufferingProbeHandler<16> *zeroLengthPtr = zeroLengthHandler.get();
        RequestHarness zeroLengthHarness(std::move(zeroLengthHandler));

        zeroLengthHarness.beginRequest();
        zeroLengthHarness.addHeader(HttpHeaderNames::ContentLength, "0");
        zeroLengthHarness.completeHeaders();
        zeroLengthHarness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, zeroLengthPtr->payloads.size());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_try_parse_header_length_accepts_complete_numeric_values);
        RUN_TEST(test_try_parse_header_length_rejects_invalid_or_partial_values);
        RUN_TEST(test_buffering_handler_does_not_buffer_when_disabled);
        RUN_TEST(test_buffering_handler_reinvokes_handle_body_on_completion_after_content_length_is_reached);
        RUN_TEST(test_buffering_handler_invokes_handle_body_from_handle_step_without_content_length);
        RUN_TEST(test_buffering_handler_truncates_to_configured_max_buffer_and_replays_on_completion);
        RUN_TEST(test_buffering_handler_ignores_empty_body_and_zero_content_length);
        return UNITY_END();
    }
}

int run_test_body_handlers()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "body handlers",
        runUnitySuite,
        localSetUp,
        localTearDown);
}