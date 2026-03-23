#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/compat/Clock.h"
#include "../../src/core/HttpTimeouts.h"
#include "../../src/pipeline/HttpPipeline.h"
#include "../../src/pipeline/IPipelineHandler.h"
#include "../../src/pipeline/PipelineError.h"
#include "../../src/pipeline/PipelineHandleClientResult.h"
#include "../../src/server/HttpServerBase.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    class ScenarioHandler : public IPipelineHandler
    {
    public:
        using Action = std::function<void(ScenarioHandler &)>;

        int onMessageBegin(const char *method, uint16_t versionMajor, uint16_t versionMinor, std::string_view url) override
        {
            method_ = method != nullptr ? method : "";
            versionMajor_ = versionMajor;
            versionMinor_ = versionMinor;
            url_ = std::string(url);
            return 0;
        }

        void setAddresses(std::string_view remoteAddress, uint16_t remotePort, std::string_view localAddress, uint16_t localPort) override
        {
            remoteAddress_ = std::string(remoteAddress);
            remotePort_ = remotePort;
            localAddress_ = std::string(localAddress);
            localPort_ = localPort;
        }

        void setResponseStreamCallback(std::function<void(std::unique_ptr<IByteSource>)> onStreamReady) override
        {
            onStreamReady_ = std::move(onStreamReady);
        }

        int onHeader(std::string_view field, std::string_view value) override
        {
            headers_.emplace_back(std::string(field), std::string(value));
            return 0;
        }

        int onHeadersComplete() override
        {
            ++headersCompleteCount_;
            if (onHeadersCompleteAction_)
            {
                onHeadersCompleteAction_(*this);
            }
            return 0;
        }

        int onBody(const uint8_t *at, std::size_t length) override
        {
            body_.append(reinterpret_cast<const char *>(at), length);
            return 0;
        }

        int onMessageComplete() override
        {
            ++messageCompleteCount_;
            if (onMessageCompleteAction_)
            {
                onMessageCompleteAction_(*this);
            }
            return 0;
        }

        void onError(PipelineError error) override
        {
            errors_.push_back(error.code());
        }

        void onResponseStarted() override
        {
            ++responseStartedCount_;
        }

        void onResponseCompleted() override
        {
            ++responseCompletedCount_;
        }

        void onClientDisconnected() override
        {
            ++clientDisconnectedCount_;
        }

        void emitResponse(std::unique_ptr<IByteSource> source)
        {
            if (onStreamReady_)
            {
                onStreamReady_(std::move(source));
            }
        }

        void setOnHeadersComplete(Action action)
        {
            onHeadersCompleteAction_ = std::move(action);
        }

        void setOnMessageComplete(Action action)
        {
            onMessageCompleteAction_ = std::move(action);
        }

        const std::string &method() const
        {
            return method_;
        }

        const std::string &url() const
        {
            return url_;
        }

        const std::vector<std::pair<std::string, std::string>> &headers() const
        {
            return headers_;
        }

        const std::string &body() const
        {
            return body_;
        }

        const std::vector<PipelineErrorCode> &errors() const
        {
            return errors_;
        }

        const std::string &remoteAddress() const
        {
            return remoteAddress_;
        }

        uint16_t remotePort() const
        {
            return remotePort_;
        }

        const std::string &localAddress() const
        {
            return localAddress_;
        }

        uint16_t localPort() const
        {
            return localPort_;
        }

        std::size_t headersCompleteCount() const
        {
            return headersCompleteCount_;
        }

        std::size_t messageCompleteCount() const
        {
            return messageCompleteCount_;
        }

        std::size_t responseStartedCount() const
        {
            return responseStartedCount_;
        }

        std::size_t responseCompletedCount() const
        {
            return responseCompletedCount_;
        }

        std::size_t clientDisconnectedCount() const
        {
            return clientDisconnectedCount_;
        }

    private:
        std::function<void(std::unique_ptr<IByteSource>)> onStreamReady_;
        Action onHeadersCompleteAction_;
        Action onMessageCompleteAction_;
        std::string method_;
        std::string url_;
        uint16_t versionMajor_ = 0;
        uint16_t versionMinor_ = 0;
        std::string remoteAddress_;
        uint16_t remotePort_ = 0;
        std::string localAddress_;
        uint16_t localPort_ = 0;
        std::vector<std::pair<std::string, std::string>> headers_;
        std::string body_;
        std::vector<PipelineErrorCode> errors_;
        std::size_t headersCompleteCount_ = 0;
        std::size_t messageCompleteCount_ = 0;
        std::size_t responseStartedCount_ = 0;
        std::size_t responseCompletedCount_ = 0;
        std::size_t clientDisconnectedCount_ = 0;
    };

    PipelineHandlerPtr makePipelineHandler(std::unique_ptr<ScenarioHandler> handler)
    {
        return PipelineHandlerPtr(
            handler.release(),
            [](IPipelineHandler *handlerPtr)
            {
                delete static_cast<ScenarioHandler *>(handlerPtr);
            });
    }

    std::unique_ptr<IByteSource> makeTextSource(std::string_view text)
    {
        return std::make_unique<TestSupport::ScriptedByteSource>(TestSupport::ScriptedByteSource::FromText(text));
    }

    class BlockingByteSource : public IByteSource
    {
    public:
        AvailableResult available() override
        {
            return TemporarilyUnavailableResult();
        }

        std::size_t read(HttpServerAdvanced::span<std::uint8_t>) override
        {
            return 0;
        }

        std::size_t peek(HttpServerAdvanced::span<std::uint8_t>) override
        {
            return 0;
        }
    };

    class PipelineHarness
    {
    public:
        explicit PipelineHarness(
            std::initializer_list<std::pair<const char *, bool>> readSteps,
            std::function<void(ScenarioHandler &)> configureHandler = nullptr,
            std::string remoteAddress = "10.0.0.8",
            uint16_t remotePort = 4567,
            std::string localAddress = "10.0.0.1",
            uint16_t localPort = 8080)
            : clock_(1000),
              server_(std::make_unique<TestSupport::FakeServer>())
        {
            timeouts_.setReadTimeout(25);
            timeouts_.setActivityTimeout(40);
            timeouts_.setTotalRequestLengthMs(200);

            auto handler = std::make_unique<ScenarioHandler>();
            if (configureHandler)
            {
                configureHandler(*handler);
            }
            handler_ = handler.get();

            auto client = std::make_unique<TestSupport::FakeClient>(
                readSteps,
                std::move(remoteAddress),
                remotePort,
                std::move(localAddress),
                localPort);
            client_ = client.get();

            pipeline_ = std::make_unique<HttpPipeline>(
                std::move(client),
                server_,
                timeouts_,
                makePipelineHandler(std::move(handler)),
                clock_);
        }

        HttpPipeline &pipeline()
        {
            return *pipeline_;
        }

        TestSupport::FakeClient &client()
        {
            return *client_;
        }

        ScenarioHandler &handler()
        {
            return *handler_;
        }

        Compat::ManualClock &clock()
        {
            return clock_;
        }

        const HttpTimeouts &timeouts() const
        {
            return timeouts_;
        }

    private:
        Compat::ManualClock clock_;
        HttpTimeouts timeouts_;
        HttpServerBase server_;
        std::unique_ptr<HttpPipeline> pipeline_;
        TestSupport::FakeClient *client_ = nullptr;
        ScenarioHandler *handler_ = nullptr;
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_http_pipeline_completes_request_response_and_reports_final_state_same_loop()
    {
        static constexpr const char *RequestText = "GET /hello HTTP/1.1\r\nHost: example.test\r\nConnection: close\r\n\r\n";
        static constexpr const char *ResponseText = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";

        PipelineHarness harness(
            {{RequestText, false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitResponse(makeTextSource(ResponseText));
                });
            });

        const PipelineHandleClientResult result = harness.pipeline().handleClient();

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Completed), static_cast<int>(result));
        TEST_ASSERT_EQUAL_UINT32(1000, harness.pipeline().startedAt());
        TEST_ASSERT_EQUAL_UINT32(harness.timeouts().getReadTimeout(), harness.client().getTimeout());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.client().timeoutHistory().size()));
        TEST_ASSERT_TRUE(harness.client().connectedCheckCount() > 0);
        TEST_ASSERT_EQUAL_STRING("GET", harness.handler().method().c_str());
        TEST_ASSERT_EQUAL_STRING("/hello", harness.handler().url().c_str());
        TEST_ASSERT_EQUAL_STRING("10.0.0.8", harness.handler().remoteAddress().c_str());
        TEST_ASSERT_EQUAL_UINT16(4567, harness.handler().remotePort());
        TEST_ASSERT_EQUAL_STRING("10.0.0.1", harness.handler().localAddress().c_str());
        TEST_ASSERT_EQUAL_UINT16(8080, harness.handler().localPort());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handler().headersCompleteCount()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handler().messageCompleteCount()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handler().responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handler().responseCompletedCount()));
        TEST_ASSERT_TRUE(harness.handler().errors().empty());
        TEST_ASSERT_TRUE(harness.pipeline().isFinished());
        TEST_ASSERT_EQUAL_STRING(ResponseText, harness.client().writtenText().c_str());
    }

    void test_http_pipeline_parses_split_request_headers_and_body()
    {
        static constexpr const char *ResponseText = "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n";

        PipelineHarness harness(
            {
                {"PO", false},
                {"ST /submit HTTP/1.1\r\nHost: ex", false},
                {"ample.test\r\nContent-Length: 5\r\n\r\nhe", false},
                {"llo", false}
            },
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitResponse(makeTextSource(ResponseText));
                });
            },
            "192.168.1.20",
            6123,
            "192.168.1.1",
            80);

        const PipelineHandleClientResult result = harness.pipeline().handleClient();

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Completed), static_cast<int>(result));
        TEST_ASSERT_EQUAL_STRING("POST", harness.handler().method().c_str());
        TEST_ASSERT_EQUAL_STRING("/submit", harness.handler().url().c_str());
        TEST_ASSERT_EQUAL_STRING("hello", harness.handler().body().c_str());
        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(harness.handler().headers().size()));
        TEST_ASSERT_EQUAL_STRING("192.168.1.20", harness.handler().remoteAddress().c_str());
        TEST_ASSERT_EQUAL_UINT16(6123, harness.handler().remotePort());
        TEST_ASSERT_TRUE(harness.pipeline().shouldKeepAlive());
        TEST_ASSERT_EQUAL_STRING(ResponseText, harness.client().writtenText().c_str());
    }

    void test_http_pipeline_reports_disconnect_before_message_completion()
    {
        PipelineHarness harness(
            {{"GET /slow HTTP/1.1\r\nHost: example.test\r\n", false}});

        PipelineHandleClientResult result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));

        harness.client().disconnect();
        result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::ClientDisconnected), static_cast<int>(result));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handler().clientDisconnectedCount()));
    }

    void test_http_pipeline_propagates_parser_errors_as_unrecoverable()
    {
        PipelineHarness harness(
            {{"GET / HTTP/1.1\r\nContent-Length: nope\r\n\r\n", false}});

        const PipelineHandleClientResult result = harness.pipeline().handleClient();

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::ErroredUnrecoverably), static_cast<int>(result));
        TEST_ASSERT_FALSE(harness.handler().errors().empty());
    }

    void test_http_pipeline_treats_partial_writes_as_unrecoverable()
    {
        static constexpr const char *RequestText = "GET /partial HTTP/1.1\r\nHost: example.test\r\n\r\n";
        static constexpr const char *ResponseText = "HTTP/1.1 200 OK\r\nContent-Length: 8\r\n\r\npartial!";

        PipelineHarness harness(
            {{RequestText, false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitResponse(makeTextSource(ResponseText));
                });
            });
        harness.client().setWriteScript({12});

        const PipelineHandleClientResult result = harness.pipeline().handleClient();

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::ErroredUnrecoverably), static_cast<int>(result));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handler().responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(harness.handler().responseCompletedCount()));
        TEST_ASSERT_EQUAL_STRING_LEN("HTTP/1.1 200 ", harness.client().writtenText().c_str(), 12);
    }

    void test_http_pipeline_times_out_waiting_for_more_request_bytes()
    {
        PipelineHarness harness(
            {
                {"GET /timeout HTTP/1.1\r\nHost: example", false},
                {"", true}
            });

        PipelineHandleClientResult result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));

        harness.clock().advanceMillis(harness.timeouts().getReadTimeout() + 1);
        result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::TimedOutUnrecoverably), static_cast<int>(result));
        TEST_ASSERT_FALSE(harness.handler().errors().empty());
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineErrorCode::Timeout), static_cast<int>(harness.handler().errors().back()));
    }

    void test_http_pipeline_times_out_waiting_for_response_bytes_and_stops_client()
    {
        static constexpr const char *RequestText = "GET /deferred HTTP/1.1\r\nHost: example.test\r\n\r\n";

        PipelineHarness harness(
            {{RequestText, false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitResponse(std::make_unique<BlockingByteSource>());
                });
            });

        PipelineHandleClientResult result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handler().messageCompleteCount()));

        harness.clock().advanceMillis(harness.timeouts().getActivityTimeout() + 1);
        result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::TimedOutUnrecoverably), static_cast<int>(result));
        TEST_ASSERT_TRUE(harness.client().stopped());
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineErrorCode::Timeout), static_cast<int>(harness.handler().errors().back()));
    }

    void test_http_pipeline_abort_helpers_drive_expected_state_transitions()
    {
        static constexpr const char *ResponseText = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";

        PipelineHarness abortedHarness({});
        abortedHarness.pipeline().abort();
        PipelineHandleClientResult result = abortedHarness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Aborted), static_cast<int>(result));

        PipelineHarness readAbortedHarness({});
        readAbortedHarness.pipeline().setResponseStream(makeTextSource(ResponseText));
        readAbortedHarness.pipeline().abortReadingRequest();
        result = readAbortedHarness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Completed), static_cast<int>(result));
        TEST_ASSERT_EQUAL_STRING(ResponseText, readAbortedHarness.client().writtenText().c_str());

        PipelineHarness writeAbortedHarness({});
        writeAbortedHarness.pipeline().setResponseStream(makeTextSource(ResponseText));
        writeAbortedHarness.pipeline().abortReadingRequest();
        writeAbortedHarness.pipeline().abortWritingResponse();
        result = writeAbortedHarness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Completed), static_cast<int>(result));
        TEST_ASSERT_TRUE(writeAbortedHarness.client().writtenText().empty());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_http_pipeline_completes_request_response_and_reports_final_state_same_loop);
        RUN_TEST(test_http_pipeline_parses_split_request_headers_and_body);
        RUN_TEST(test_http_pipeline_reports_disconnect_before_message_completion);
        RUN_TEST(test_http_pipeline_propagates_parser_errors_as_unrecoverable);
        RUN_TEST(test_http_pipeline_treats_partial_writes_as_unrecoverable);
        RUN_TEST(test_http_pipeline_times_out_waiting_for_more_request_bytes);
        RUN_TEST(test_http_pipeline_times_out_waiting_for_response_bytes_and_stops_client);
        RUN_TEST(test_http_pipeline_abort_helpers_drive_expected_state_transitions);
        return UNITY_END();
    }
}

int run_test_pipeline()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "pipeline",
        runUnitySuite,
        localSetUp,
        localTearDown);
}