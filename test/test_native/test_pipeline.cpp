#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/compat/Clock.h"
#include "../../src/core/HttpRequest.h"
#include "../../src/core/HttpTimeouts.h"
#include "../../src/handlers/HttpHandler.h"
#include "../../src/pipeline/HttpPipeline.h"
#include "../../src/pipeline/IPipelineHandler.h"
#include "../../src/pipeline/PipelineError.h"
#include "../../src/pipeline/PipelineHandleClientResult.h"
#include "../../src/response/StringResponse.h"
#include "../../src/server/HttpServerBase.h"
#include "../../src/websocket/WebSocketCallbacks.h"
#include "../../src/websocket/WebSocketRoute.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
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
            pendingResult_ = RequestHandlingResult::response(std::move(source));
        }

        void emitUpgrade(std::unique_ptr<IConnectionSession> session)
        {
            pendingResult_ = RequestHandlingResult::upgrade(std::move(session));
        }

        void emitNoResponse()
        {
            pendingResult_ = RequestHandlingResult::noResponse();
        }

        void emitError(PipelineErrorCode errorCode)
        {
            pendingResult_ = RequestHandlingResult::errorResult(PipelineError(errorCode));
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

        bool hasPendingResult() const override
        {
            return pendingResult_.hasValue();
        }

        RequestHandlingResult takeResult() override
        {
            RequestHandlingResult result = std::move(pendingResult_);
            pendingResult_ = RequestHandlingResult();
            return result;
        }

    private:
        Action onHeadersCompleteAction_;
        Action onMessageCompleteAction_;
        RequestHandlingResult pendingResult_;
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

    class StubConnectionSession : public IConnectionSession
    {
    public:
        explicit StubConnectionSession(std::vector<ConnectionSessionResult> scriptedResults, std::size_t *handleCount)
            : scriptedResults_(std::move(scriptedResults)),
              handleCount_(handleCount)
        {
        }

        ConnectionSessionResult handle(IClient &, const Compat::Clock &) override
        {
            if (handleCount_ != nullptr)
            {
                ++(*handleCount_);
            }
            if (scriptedIndex_ >= scriptedResults_.size())
            {
                return ConnectionSessionResult::Continue;
            }

            return scriptedResults_[scriptedIndex_++];
        }

    private:
        std::vector<ConnectionSessionResult> scriptedResults_;
        std::size_t *handleCount_ = nullptr;
        std::size_t scriptedIndex_ = 0;
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
                [this, configureHandler]()
                {
                    auto handler = std::make_unique<ScenarioHandler>();
                    if (configureHandler)
                    {
                        configureHandler(*handler);
                    }
                    handler_ = handler.get();
                    handlerHistory_.push_back(handler_);
                    return makePipelineHandler(std::move(handler));
                },
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

        ScenarioHandler &handlerAt(std::size_t index)
        {
            return *handlerHistory_.at(index);
        }

        std::size_t handlerCount() const
        {
            return handlerHistory_.size();
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
        std::vector<ScenarioHandler *> handlerHistory_;
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    bool hasBinarySuffix(const std::string &value, span<const std::uint8_t> suffix)
    {
        if (suffix.size() > value.size())
        {
            return false;
        }

        const auto *begin = reinterpret_cast<const std::uint8_t *>(value.data() + (value.size() - suffix.size()));
        for (std::size_t i = 0; i < suffix.size(); ++i)
        {
            if (begin[i] != suffix[i])
            {
                return false;
            }
        }

        return true;
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

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_UINT32(1000, harness.pipeline().startedAt());
        TEST_ASSERT_EQUAL_UINT32(harness.timeouts().getReadTimeout(), harness.client().getTimeout());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.client().timeoutHistory().size()));
        TEST_ASSERT_TRUE(harness.client().connectedCheckCount() > 0);
        TEST_ASSERT_EQUAL_STRING("GET", harness.handlerAt(0).method().c_str());
        TEST_ASSERT_EQUAL_STRING("/hello", harness.handlerAt(0).url().c_str());
        TEST_ASSERT_EQUAL_STRING("10.0.0.8", harness.handlerAt(0).remoteAddress().c_str());
        TEST_ASSERT_EQUAL_UINT16(4567, harness.handlerAt(0).remotePort());
        TEST_ASSERT_EQUAL_STRING("10.0.0.1", harness.handlerAt(0).localAddress().c_str());
        TEST_ASSERT_EQUAL_UINT16(8080, harness.handlerAt(0).localPort());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).headersCompleteCount()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).messageCompleteCount()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseCompletedCount()));
        TEST_ASSERT_TRUE(harness.handlerAt(0).errors().empty());
        TEST_ASSERT_FALSE(harness.pipeline().isFinished());
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::ReadingHttpRequest), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_STRING(ResponseText, harness.client().writtenText().c_str());
        TEST_ASSERT_EQUAL_UINT64(2, harness.handlerCount());
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

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_STRING("POST", harness.handlerAt(0).method().c_str());
        TEST_ASSERT_EQUAL_STRING("/submit", harness.handlerAt(0).url().c_str());
        TEST_ASSERT_EQUAL_STRING("hello", harness.handlerAt(0).body().c_str());
        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(harness.handlerAt(0).headers().size()));
        TEST_ASSERT_EQUAL_STRING("192.168.1.20", harness.handlerAt(0).remoteAddress().c_str());
        TEST_ASSERT_EQUAL_UINT16(6123, harness.handlerAt(0).remotePort());
        TEST_ASSERT_FALSE(harness.pipeline().shouldKeepAlive());
        TEST_ASSERT_EQUAL_STRING(ResponseText, harness.client().writtenText().c_str());
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::ReadingHttpRequest), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_UINT64(2, harness.handlerCount());
    }

    void test_http_pipeline_reenters_keep_alive_after_no_response_result()
    {
        static constexpr const char *RequestText = "GET /skip HTTP/1.1\r\nHost: example.test\r\n\r\n";

        PipelineHarness harness(
            {{RequestText, false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitNoResponse();
                });
            });

        const PipelineHandleClientResult result = harness.pipeline().handleClient();

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).messageCompleteCount()));
        TEST_ASSERT_TRUE(harness.handlerAt(0).errors().empty());
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::ReadingHttpRequest), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_UINT64(2, harness.handlerCount());
        TEST_ASSERT_TRUE(harness.client().writtenText().empty());
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(harness.handlerAt(0).responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(harness.handlerAt(0).responseCompletedCount()));
    }

    void test_http_pipeline_rejects_null_response_stream_as_unrecoverable()
    {
        static constexpr const char *RequestText = "GET /null HTTP/1.1\r\nHost: example.test\r\n\r\n";

        PipelineHarness harness(
            {{RequestText, false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitResponse(std::unique_ptr<IByteSource>());
                });
            });

        const PipelineHandleClientResult result = harness.pipeline().handleClient();

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::ErroredUnrecoverably), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::Error), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_TRUE(harness.client().writtenText().empty());
    }

    void test_http_pipeline_rejects_explicit_handler_error_result_as_unrecoverable()
    {
        static constexpr const char *RequestText = "GET /error HTTP/1.1\r\nHost: example.test\r\n\r\n";

        PipelineHarness harness(
            {{RequestText, false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitError(PipelineErrorCode::BadRequest);
                });
            });

        const PipelineHandleClientResult result = harness.pipeline().handleClient();

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::ErroredUnrecoverably), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::Error), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).messageCompleteCount()));
        TEST_ASSERT_TRUE(harness.client().writtenText().empty());
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
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::Closing), static_cast<int>(harness.pipeline().connectionState()));
    }

    void test_http_pipeline_propagates_parser_errors_as_unrecoverable()
    {
        PipelineHarness harness(
            {{"GET / HTTP/1.1\r\nContent-Length: nope\r\n\r\n", false}});

        const PipelineHandleClientResult result = harness.pipeline().handleClient();

        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::ErroredUnrecoverably), static_cast<int>(result));
        TEST_ASSERT_FALSE(harness.handler().errors().empty());
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::Error), static_cast<int>(harness.pipeline().connectionState()));
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
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(harness.handlerAt(0).responseCompletedCount()));
        TEST_ASSERT_EQUAL_STRING_LEN("HTTP/1.1 200 ", harness.client().writtenText().c_str(), 12);
    }

    void test_http_pipeline_transitions_into_upgraded_session_and_completes_when_session_finishes()
    {
        static constexpr const char *RequestText = "GET /ws HTTP/1.1\r\nHost: example.test\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n\r\n";

        std::size_t sessionHandleCount = 0;
        PipelineHarness harness(
            {{RequestText, false}},
            [&sessionHandleCount](ScenarioHandler &handler)
            {
                handler.setOnHeadersComplete([&sessionHandleCount](ScenarioHandler &scenario)
                {
                    auto ownedSession = std::make_unique<StubConnectionSession>(std::vector<ConnectionSessionResult>{ConnectionSessionResult::Continue, ConnectionSessionResult::Completed}, &sessionHandleCount);
                    scenario.emitUpgrade(std::move(ownedSession));
                });
            });

        PipelineHandleClientResult result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_UINT64(1, sessionHandleCount);

        result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Completed), static_cast<int>(result));
        TEST_ASSERT_EQUAL_UINT64(2, sessionHandleCount);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::Completed), static_cast<int>(harness.pipeline().connectionState()));
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
        PipelineHarness abortedHarness({});
        abortedHarness.pipeline().abort();
        PipelineHandleClientResult result = abortedHarness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Aborted), static_cast<int>(result));

        PipelineHarness readAbortedHarness({{"GET /pending HTTP/1.1\r\nHost: example.test\r\n", false}});
        readAbortedHarness.pipeline().abortReadingRequest();
        result = readAbortedHarness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));

        PipelineHarness writeAbortedHarness(
            {{"GET /ok HTTP/1.1\r\nHost: example.test\r\nConnection: close\r\n\r\n", false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitResponse(std::make_unique<BlockingByteSource>());
                });
            });
        result = writeAbortedHarness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        writeAbortedHarness.pipeline().abortWritingResponse();
        result = writeAbortedHarness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_TRUE(writeAbortedHarness.client().writtenText().empty());
    }

    void test_http_pipeline_progresses_response_writing_across_temporarily_unavailable_cycles()
    {
        static constexpr const char *RequestText = "GET /stream HTTP/1.1\r\nHost: example.test\r\nConnection: close\r\n\r\n";

        PipelineHarness harness(
            {{RequestText, false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitResponse(std::make_unique<TestSupport::ScriptedByteSource>(
                        std::initializer_list<std::pair<const char *, bool>>{{"ab", false}, {"", true}, {"cd", false}, {"", true}, {"ef", false}}));
                });
            });

        PipelineHandleClientResult result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::WritingHttpResponse), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_STRING("ab", harness.client().writtenText().c_str());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(harness.handlerAt(0).responseCompletedCount()));

        result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::WritingHttpResponse), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_STRING("abcd", harness.client().writtenText().c_str());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(harness.handlerAt(0).responseCompletedCount()));

        result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::ReadingHttpRequest), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_STRING("abcdef", harness.client().writtenText().c_str());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseCompletedCount()));
    }

    void test_http_pipeline_reports_disconnect_during_writing_and_does_not_complete_response()
    {
        static constexpr const char *RequestText = "GET /disconnect-while-writing HTTP/1.1\r\nHost: example.test\r\nConnection: close\r\n\r\n";

        PipelineHarness harness(
            {{RequestText, false}},
            [](ScenarioHandler &handler)
            {
                handler.setOnMessageComplete([](ScenarioHandler &scenario)
                {
                    scenario.emitResponse(std::make_unique<TestSupport::ScriptedByteSource>(
                        std::initializer_list<std::pair<const char *, bool>>{{"ab", false}, {"", true}, {"cd", false}}));
                });
            });

        PipelineHandleClientResult result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::WritingHttpResponse), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_STRING("ab", harness.client().writtenText().c_str());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(harness.handlerAt(0).responseCompletedCount()));

        harness.client().disconnect();
        result = harness.pipeline().handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::ClientDisconnected), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::Closing), static_cast<int>(harness.pipeline().connectionState()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).clientDisconnectedCount()));
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(harness.handlerAt(0).responseStartedCount()));
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(harness.handlerAt(0).responseCompletedCount()));
        TEST_ASSERT_EQUAL_STRING("ab", harness.client().writtenText().c_str());
    }

    void test_http_pipeline_websocket_accept_path_transitions_to_upgraded_session_and_writes_handshake()
    {
        static constexpr const char *RequestText =
            "GET /chat HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "Connection: Upgrade\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "\r\n";

        Compat::ManualClock clock(1000);
        HttpTimeouts timeouts;
        timeouts.setReadTimeout(25);
        timeouts.setActivityTimeout(40);
        timeouts.setTotalRequestLengthMs(200);

        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory requestFactory(
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return nullptr;
            });
        std::vector<WebSocketRoute> routes = {WebSocketRoute{"/chat", WebSocketCallbacks{}}};

        auto client = std::make_unique<TestSupport::FakeClient>(std::initializer_list<std::pair<const char *, bool>>{{RequestText, false}});
        TestSupport::FakeClient *clientPtr = client.get();

        HttpPipeline pipeline(
            std::move(client),
            server,
            timeouts,
            [&server, &requestFactory, &routes]()
            {
                return HttpRequest::createPipelineHandler(server, requestFactory, &routes);
            },
            clock);

        const PipelineHandleClientResult firstResult = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(firstResult));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));
        TEST_ASSERT_NOT_NULL(strstr(clientPtr->writtenText().c_str(), "HTTP/1.1 101 Switching Protocols"));
        TEST_ASSERT_NOT_NULL(strstr(clientPtr->writtenText().c_str(), "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo="));

        const PipelineHandleClientResult secondResult = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(secondResult));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));
    }

    void test_http_pipeline_websocket_rejection_path_stays_http_and_never_enters_upgraded_session()
    {
        static constexpr const char *RequestText =
            "GET /chat HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "Connection: Upgrade\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 12\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "\r\n";

        Compat::ManualClock clock(1000);
        HttpTimeouts timeouts;
        timeouts.setReadTimeout(25);
        timeouts.setActivityTimeout(40);
        timeouts.setTotalRequestLengthMs(200);

        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory requestFactory(
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return nullptr;
            });
        std::vector<WebSocketRoute> routes = {WebSocketRoute{"/chat", WebSocketCallbacks{}}};

        auto client = std::make_unique<TestSupport::FakeClient>(std::initializer_list<std::pair<const char *, bool>>{{RequestText, false}});
        TestSupport::FakeClient *clientPtr = client.get();

        HttpPipeline pipeline(
            std::move(client),
            server,
            timeouts,
            [&server, &requestFactory, &routes]()
            {
                return HttpRequest::createPipelineHandler(server, requestFactory, &routes);
            },
            clock);

        const PipelineHandleClientResult result = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_TRUE(pipeline.connectionState() != HttpPipeline::ConnectionState::UpgradedSessionActive);
        TEST_ASSERT_NOT_NULL(strstr(clientPtr->writtenText().c_str(), "HTTP/1.1 400 Bad Request"));
    }

    void test_http_pipeline_websocket_session_runtime_sends_pong_for_ping_frames()
    {
        static constexpr const char *RequestText =
            "GET /chat HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "Connection: Upgrade\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "\r\n";

        static constexpr const char *PingFrame = "\x89\x82\x01\x02\x03\x04\x69\x6B";

        Compat::ManualClock clock(1000);
        HttpTimeouts timeouts;
        timeouts.setReadTimeout(25);
        timeouts.setActivityTimeout(40);
        timeouts.setTotalRequestLengthMs(200);

        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory requestFactory(
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return nullptr;
            });
        std::vector<WebSocketRoute> routes = {WebSocketRoute{"/chat", WebSocketCallbacks{}}};

        auto client = std::make_unique<TestSupport::FakeClient>(
            std::initializer_list<std::pair<const char *, bool>>{{RequestText, false}, {PingFrame, false}});
        TestSupport::FakeClient *clientPtr = client.get();

        HttpPipeline pipeline(
            std::move(client),
            server,
            timeouts,
            [&server, &requestFactory, &routes]()
            {
                return HttpRequest::createPipelineHandler(server, requestFactory, &routes);
            },
            clock);

        const PipelineHandleClientResult firstResult = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(firstResult));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));

        const PipelineHandleClientResult secondResult = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(secondResult));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));

        const std::uint8_t expectedPong[] = {0x8A, 0x02, 'h', 'i'};
        TEST_ASSERT_TRUE(hasBinarySuffix(clientPtr->writtenText(), span<const std::uint8_t>(expectedPong, sizeof(expectedPong))));
    }

    void test_http_pipeline_websocket_session_runtime_completes_after_close_handshake()
    {
        static constexpr const char *RequestText =
            "GET /chat HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "Connection: Upgrade\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "\r\n";

        static constexpr const char *CloseFrame = "\x88\x82\x01\x02\x03\x04\x02\xEA";

        Compat::ManualClock clock(1000);
        HttpTimeouts timeouts;
        timeouts.setReadTimeout(25);
        timeouts.setActivityTimeout(40);
        timeouts.setTotalRequestLengthMs(200);

        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory requestFactory(
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return nullptr;
            });
        std::vector<WebSocketRoute> routes = {WebSocketRoute{"/chat", WebSocketCallbacks{}}};

        auto client = std::make_unique<TestSupport::FakeClient>(
            std::initializer_list<std::pair<const char *, bool>>{{RequestText, false}, {CloseFrame, false}});
        TestSupport::FakeClient *clientPtr = client.get();

        HttpPipeline pipeline(
            std::move(client),
            server,
            timeouts,
            [&server, &requestFactory, &routes]()
            {
                return HttpRequest::createPipelineHandler(server, requestFactory, &routes);
            },
            clock);

        const PipelineHandleClientResult firstResult = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(firstResult));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));

        const PipelineHandleClientResult secondResult = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Completed), static_cast<int>(secondResult));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::Completed), static_cast<int>(pipeline.connectionState()));

        const std::uint8_t expectedClose[] = {0x88, 0x02, 0x03, 0xE8};
        TEST_ASSERT_TRUE(hasBinarySuffix(clientPtr->writtenText(), span<const std::uint8_t>(expectedClose, sizeof(expectedClose))));
    }

    void test_http_pipeline_websocket_session_runtime_maps_protocol_error_to_close_frame()
    {
        static constexpr const char *RequestText =
            "GET /chat HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "Connection: Upgrade\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "\r\n";
        static constexpr const char *InvalidUnmaskedFrame = "\x81\x02ok";

        Compat::ManualClock clock(1000);
        HttpTimeouts timeouts;
        timeouts.setReadTimeout(25);
        timeouts.setActivityTimeout(40);
        timeouts.setTotalRequestLengthMs(200);

        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory requestFactory(
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return nullptr;
            });
        std::vector<WebSocketRoute> routes = {WebSocketRoute{"/chat", WebSocketCallbacks{}}};

        auto client = std::make_unique<TestSupport::FakeClient>(
            std::initializer_list<std::pair<const char *, bool>>{{RequestText, false}, {InvalidUnmaskedFrame, false}});
        TestSupport::FakeClient *clientPtr = client.get();

        HttpPipeline pipeline(
            std::move(client),
            server,
            timeouts,
            [&server, &requestFactory, &routes]()
            {
                return HttpRequest::createPipelineHandler(server, requestFactory, &routes);
            },
            clock);

        const PipelineHandleClientResult firstResult = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(firstResult));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));

        const PipelineHandleClientResult secondResult = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(secondResult));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));

        const std::uint8_t expectedClose[] = {0x88, 0x02, 0x03, 0xEA};
        TEST_ASSERT_TRUE(hasBinarySuffix(clientPtr->writtenText(), span<const std::uint8_t>(expectedClose, sizeof(expectedClose))));
    }

    void test_http_pipeline_websocket_session_runtime_resumes_partial_handshake_writes_across_loops()
    {
        static constexpr const char *RequestText =
            "GET /chat HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "Connection: Upgrade\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "\r\n";

        Compat::ManualClock clock(1000);
        HttpTimeouts timeouts;
        timeouts.setReadTimeout(25);
        timeouts.setActivityTimeout(40);
        timeouts.setTotalRequestLengthMs(200);

        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory requestFactory(
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return nullptr;
            });
        std::vector<WebSocketRoute> routes = {WebSocketRoute{"/chat", WebSocketCallbacks{}}};

        auto client = std::make_unique<TestSupport::FakeClient>(std::initializer_list<std::pair<const char *, bool>>{{RequestText, false}});
        TestSupport::FakeClient *clientPtr = client.get();
        clientPtr->setWriteScript({8, 0, 8, 0, 256});

        HttpPipeline pipeline(
            std::move(client),
            server,
            timeouts,
            [&server, &requestFactory, &routes]()
            {
                return HttpRequest::createPipelineHandler(server, requestFactory, &routes);
            },
            clock);

        PipelineHandleClientResult result = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));
        const std::size_t firstWriteBytes = clientPtr->writtenText().size();
        TEST_ASSERT_TRUE(firstWriteBytes <= 8U);

        result = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));
        TEST_ASSERT_TRUE(clientPtr->writtenText().size() > firstWriteBytes);

        result = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::UpgradedSessionActive), static_cast<int>(pipeline.connectionState()));
        TEST_ASSERT_NOT_NULL(strstr(clientPtr->writtenText().c_str(), "HTTP/1.1 101 Switching Protocols"));
    }

    void test_http_pipeline_websocket_session_runtime_invokes_callbacks_in_expected_order()
    {
        static constexpr const char *RequestText =
            "GET /chat HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "Connection: Upgrade\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "\r\n";
        static constexpr const char *TextFrame = "\x81\x82\x01\x02\x03\x04\x69\x6B";
        static constexpr const char *BinaryFrame = "\x82\x82\x01\x02\x03\x04\x40\x40";
        static constexpr const char *CloseFrame = "\x88\x82\x01\x02\x03\x04\x02\xEA";

        std::vector<std::string> callbackEvents;

        Compat::ManualClock clock(1000);
        HttpTimeouts timeouts;
        timeouts.setReadTimeout(25);
        timeouts.setActivityTimeout(40);
        timeouts.setTotalRequestLengthMs(200);

        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory requestFactory(
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return nullptr;
            });

        WebSocketCallbacks callbacks;
        callbacks.onOpen = [&callbackEvents]()
        {
            callbackEvents.push_back("open");
        };
        callbacks.onText = [&callbackEvents](std::string_view text)
        {
            callbackEvents.push_back(std::string("text:") + std::string(text));
        };
        callbacks.onBinary = [&callbackEvents](span<const std::uint8_t> payload)
        {
            callbackEvents.push_back(std::string("binary:") + std::string(reinterpret_cast<const char *>(payload.data()), payload.size()));
        };
        callbacks.onClose = [&callbackEvents](std::uint16_t code, std::string_view)
        {
            callbackEvents.push_back(std::string("close:") + std::to_string(code));
        };
        callbacks.onError = [&callbackEvents](std::string_view message)
        {
            callbackEvents.push_back(std::string("error:") + std::string(message));
        };

        std::vector<WebSocketRoute> routes = {WebSocketRoute{"/chat", callbacks}};

        auto client = std::make_unique<TestSupport::FakeClient>(
            std::initializer_list<std::pair<const char *, bool>>{{RequestText, false}, {TextFrame, false}, {BinaryFrame, false}, {CloseFrame, false}});

        HttpPipeline pipeline(
            std::move(client),
            server,
            timeouts,
            [&server, &requestFactory, &routes]()
            {
                return HttpRequest::createPipelineHandler(server, requestFactory, &routes);
            },
            clock);

        PipelineHandleClientResult result = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));

        result = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Completed), static_cast<int>(result));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpPipeline::ConnectionState::Completed), static_cast<int>(pipeline.connectionState()));

        TEST_ASSERT_EQUAL_UINT64(4, callbackEvents.size());
        TEST_ASSERT_EQUAL_STRING("open", callbackEvents[0].c_str());
        TEST_ASSERT_EQUAL_STRING("text:hi", callbackEvents[1].c_str());
        TEST_ASSERT_EQUAL_STRING("binary:AB", callbackEvents[2].c_str());
        TEST_ASSERT_EQUAL_STRING("close:1000", callbackEvents[3].c_str());
    }

    void test_http_pipeline_websocket_upgrade_candidate_without_matching_route_falls_back_to_http_handler()
    {
        static constexpr const char *RequestText =
            "GET /chat HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "Connection: Upgrade\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "\r\n";

        Compat::ManualClock clock(1000);
        HttpTimeouts timeouts;
        timeouts.setReadTimeout(25);
        timeouts.setActivityTimeout(40);
        timeouts.setTotalRequestLengthMs(200);

        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory requestFactory(
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return std::make_unique<HttpHandler>(
                    [](HttpRequest &)
                    {
                        return StringResponse::create(HttpStatus::Ok(), "http-fallback", {});
                    },
                    [](const HttpRequest &)
                    {
                        return true;
                    });
            });

        std::vector<WebSocketRoute> routes = {WebSocketRoute{"/other", WebSocketCallbacks{}}};

        auto client = std::make_unique<TestSupport::FakeClient>(std::initializer_list<std::pair<const char *, bool>>{{RequestText, false}});
        TestSupport::FakeClient *clientPtr = client.get();

        HttpPipeline pipeline(
            std::move(client),
            server,
            timeouts,
            [&server, &requestFactory, &routes]()
            {
                return HttpRequest::createPipelineHandler(server, requestFactory, &routes);
            },
            clock);

        const PipelineHandleClientResult result = pipeline.handleClient();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PipelineHandleClientResult::Processing), static_cast<int>(result));
        TEST_ASSERT_TRUE(pipeline.connectionState() != HttpPipeline::ConnectionState::UpgradedSessionActive);
        TEST_ASSERT_NOT_NULL(strstr(clientPtr->writtenText().c_str(), "HTTP/1.1 200 OK"));
        TEST_ASSERT_NOT_NULL(strstr(clientPtr->writtenText().c_str(), "http-fallback"));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_http_pipeline_completes_request_response_and_reports_final_state_same_loop);
        RUN_TEST(test_http_pipeline_parses_split_request_headers_and_body);
        RUN_TEST(test_http_pipeline_reenters_keep_alive_after_no_response_result);
        RUN_TEST(test_http_pipeline_rejects_null_response_stream_as_unrecoverable);
        RUN_TEST(test_http_pipeline_rejects_explicit_handler_error_result_as_unrecoverable);
        RUN_TEST(test_http_pipeline_reports_disconnect_before_message_completion);
        RUN_TEST(test_http_pipeline_propagates_parser_errors_as_unrecoverable);
        RUN_TEST(test_http_pipeline_treats_partial_writes_as_unrecoverable);
        RUN_TEST(test_http_pipeline_transitions_into_upgraded_session_and_completes_when_session_finishes);
        RUN_TEST(test_http_pipeline_times_out_waiting_for_more_request_bytes);
        RUN_TEST(test_http_pipeline_times_out_waiting_for_response_bytes_and_stops_client);
        RUN_TEST(test_http_pipeline_abort_helpers_drive_expected_state_transitions);
        RUN_TEST(test_http_pipeline_progresses_response_writing_across_temporarily_unavailable_cycles);
        RUN_TEST(test_http_pipeline_reports_disconnect_during_writing_and_does_not_complete_response);
        RUN_TEST(test_http_pipeline_websocket_accept_path_transitions_to_upgraded_session_and_writes_handshake);
        RUN_TEST(test_http_pipeline_websocket_rejection_path_stays_http_and_never_enters_upgraded_session);
        RUN_TEST(test_http_pipeline_websocket_session_runtime_sends_pong_for_ping_frames);
        RUN_TEST(test_http_pipeline_websocket_session_runtime_completes_after_close_handshake);
        RUN_TEST(test_http_pipeline_websocket_session_runtime_maps_protocol_error_to_close_frame);
        RUN_TEST(test_http_pipeline_websocket_session_runtime_resumes_partial_handshake_writes_across_loops);
        RUN_TEST(test_http_pipeline_websocket_session_runtime_invokes_callbacks_in_expected_order);
        RUN_TEST(test_http_pipeline_websocket_upgrade_candidate_without_matching_route_falls_back_to_http_handler);
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