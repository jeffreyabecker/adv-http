#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/compat/Clock.h"
#include "../../src/core/HttpContext.h"
#include "../../src/core/HttpContextPhase.h"
#include "../../src/handlers/HttpHandler.h"
#include "../../src/pipeline/RequestParser.h"
#include "../../src/routing/HandlerMatcher.h"
#include "../../src/server/HttpServerBase.h"
#include "../../src/websocket/WebSocketCallbacks.h"
#include "../../src/websocket/WebSocketUpgradeHandler.h"

#include <memory>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    class CapturingHandler : public IHttpHandler
    {
    public:
        HandlerResult handleStep(HttpContext &context) override
        {
            seenMethods_.push_back(std::string(context.methodView()));
            seenPhases_.push_back(context.completedPhases());
            return nullptr;
        }

        void handleBodyChunk(HttpContext &context, const uint8_t *, std::size_t) override
        {
            bodyChunkMethods_.push_back(std::string(context.methodView()));
            bodyChunkPhases_.push_back(context.completedPhases());
        }

        const std::vector<std::string> &seenMethods() const
        {
            return seenMethods_;
        }

        const std::vector<HttpContextPhaseFlags> &seenPhases() const
        {
            return seenPhases_;
        }

        const std::vector<std::string> &bodyChunkMethods() const
        {
            return bodyChunkMethods_;
        }

        const std::vector<HttpContextPhaseFlags> &bodyChunkPhases() const
        {
            return bodyChunkPhases_;
        }

    private:
        std::vector<std::string> seenMethods_;
        std::vector<HttpContextPhaseFlags> seenPhases_;
        std::vector<std::string> bodyChunkMethods_;
        std::vector<HttpContextPhaseFlags> bodyChunkPhases_;
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    TestSupport::RecordingRequestHandlerFactory::HandlerFactoryCallback createWebSocketUpgradeHandlerFactory(
        std::string_view path,
        WebSocketCallbacks callbacks = {})
    {
        return [registeredPath = std::string(path), callbacks = std::move(callbacks)](HttpContext &)
        {
            return std::make_unique<HttpHandler>(
                [registeredPath, callbacks](HttpContext &context) mutable -> IHttpHandler::HandlerResult
                {
                    if (!WebSocketUpgradeHandler::isWebSocketUpgradeCandidate(context) || !defaultCheckUriPattern(context.uriView().path(), registeredPath))
                    {
                        return nullptr;
                    }

                    WebSocketUpgradeHandler upgradeHandler;
                    return upgradeHandler.handle(context, callbacks);
                });
        };
    }

    void test_http_context_preserves_custom_method_through_factory_and_handler_steps()
    {
        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        std::unique_ptr<CapturingHandler> capturingHandler = std::make_unique<CapturingHandler>();
        CapturingHandler *capturingHandlerPtr = capturingHandler.get();
        std::vector<std::string> factoryMethods;
        std::vector<HttpContextPhaseFlags> factoryPhases;

        TestSupport::RecordingRequestHandlerFactory factory(
            [&factoryMethods, &factoryPhases, &capturingHandler](HttpContext &context) -> std::unique_ptr<IHttpHandler>
            {
                factoryMethods.push_back(std::string(context.methodView()));
                factoryPhases.push_back(context.completedPhases());
                return std::move(capturingHandler);
            });

        auto pipelineHandler = HttpContext::createPipelineHandler(server, factory);

        const std::uint8_t body[] = {'o', 'k'};

        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onMessageBegin("MKCOL", 1, 1, "/dav/collection"));
        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onHeader("Host", "example.test"));
        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onHeadersComplete());
        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onBody(body, sizeof(body)));
        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onMessageComplete());

        TEST_ASSERT_EQUAL_UINT64(1, factory.createCount());
        TEST_ASSERT_EQUAL_UINT64(1, factoryMethods.size());
        TEST_ASSERT_EQUAL_STRING("MKCOL", factoryMethods[0].c_str());
        TEST_ASSERT_TRUE((factoryPhases[0] & HttpContextPhase::CompletedStartingLine) != 0);
        TEST_ASSERT_NOT_NULL(factory.lastCreateContext());
        TEST_ASSERT_EQUAL_STRING("MKCOL", factory.lastCreateContext()->method());
        TEST_ASSERT_EQUAL_STRING("/dav/collection", std::string(factory.lastCreateContext()->url()).c_str());

        TEST_ASSERT_NOT_NULL(capturingHandlerPtr);
        TEST_ASSERT_EQUAL_UINT64(5, capturingHandlerPtr->seenMethods().size());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[0].c_str());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[1].c_str());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[2].c_str());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[3].c_str());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[4].c_str());
        TEST_ASSERT_TRUE((capturingHandlerPtr->seenPhases()[0] & HttpContextPhase::CompletedStartingLine) != 0);
        TEST_ASSERT_TRUE((capturingHandlerPtr->seenPhases()[1] & HttpContextPhase::CompletedReadingHeaders) != 0);
        TEST_ASSERT_TRUE((capturingHandlerPtr->seenPhases()[2] & HttpContextPhase::BeginReadingBody) != 0);
        TEST_ASSERT_EQUAL_UINT64(1, capturingHandlerPtr->bodyChunkMethods().size());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->bodyChunkMethods()[0].c_str());
        TEST_ASSERT_TRUE((capturingHandlerPtr->bodyChunkPhases()[0] & HttpContextPhase::BeginReadingBody) != 0);
        TEST_ASSERT_TRUE((capturingHandlerPtr->seenPhases()[4] & HttpContextPhase::CompletedReadingMessage) != 0);
        TEST_ASSERT_TRUE((factory.lastCreateContext()->completedPhases() & HttpContextPhase::CompletedReadingMessage) != 0);
    }

    std::string ExecuteAndCaptureResponseText(
        std::string_view method,
        std::string_view path,
        const std::vector<std::pair<std::string, std::string>> &headers,
        TestSupport::RecordingRequestHandlerFactory &factory,
        RequestHandlingResult::Kind expectedKind)
    {
        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        auto pipelineHandler = HttpContext::createPipelineHandler(server, factory);
        RequestParser parser(*pipelineHandler);

        std::string request;
        request.append(method);
        request.append(" ");
        request.append(path);
        request.append(" HTTP/1.1\r\n");
        for (const auto &header : headers)
        {
            request.append(header.first);
            request.append(": ");
            request.append(header.second);
            request.append("\r\n");
        }
        request.append("\r\n");

        TEST_ASSERT_EQUAL_UINT64(request.size(), parser.execute(reinterpret_cast<const std::uint8_t *>(request.data()), request.size()));
        TEST_ASSERT_TRUE(pipelineHandler->hasPendingResult());

        RequestHandlingResult result = pipelineHandler->takeResult();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(expectedKind), static_cast<int>(result.kind));

        if (result.kind == RequestHandlingResult::Kind::Response)
        {
            TEST_ASSERT_NOT_NULL(result.responseStream.get());
            TEST_ASSERT_NULL(result.upgradedSession.get());
            return TestSupport::ReadByteSourceAsStdString(*result.responseStream);
        }

        return std::string();
    }

    void test_http_context_websocket_upgrade_accepts_split_request_and_returns_upgrade_session()
    {
        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        TestSupport::RecordingRequestHandlerFactory factory(
            createWebSocketUpgradeHandlerFactory("/chat"));

        auto pipelineHandler = HttpContext::createPipelineHandler(server, factory);
        RequestParser parser(*pipelineHandler);

        const std::vector<std::string> chunks = {
            "GET /chat HTTP/1.1\r\nHo",
            "st: example.test\r\ncOnNe",
            "ction: Up",
            "grade\r\nupgra",
            "de: webs",
            "ocket\r\nSec-Web",
            "Socket-Vers",
            "ion: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n"
        };

        for (const auto &chunk : chunks)
        {
            TEST_ASSERT_EQUAL_UINT64(chunk.size(), parser.execute(reinterpret_cast<const std::uint8_t *>(chunk.data()), chunk.size()));
        }

        TEST_ASSERT_TRUE(factory.createCount() >= 1);
        TEST_ASSERT_TRUE(pipelineHandler->hasPendingResult());

        RequestHandlingResult result = pipelineHandler->takeResult();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(RequestHandlingResult::Kind::Upgrade), static_cast<int>(result.kind));
        TEST_ASSERT_NOT_NULL(result.upgradedSession.get());
        TEST_ASSERT_NULL(result.responseStream.get());

        TestSupport::FakeClient client;
        Compat::ManualClock clock(1000);
        const ConnectionSessionResult firstStep = result.upgradedSession->handle(client, clock);

        TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionSessionResult::Continue), static_cast<int>(firstStep));
        TEST_ASSERT_EQUAL_STRING(
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n",
            client.writtenText().c_str());
    }

    void test_http_context_websocket_upgrade_rejects_invalid_requests_with_deterministic_statuses()
    {
        {
            TestSupport::RecordingRequestHandlerFactory factory(createWebSocketUpgradeHandlerFactory("/chat"));
            const std::string responseText = ExecuteAndCaptureResponseText(
                "POST",
                "/chat",
                {
                    {"Host", "example.test"},
                    {"Connection", "Upgrade"},
                    {"Upgrade", "websocket"},
                    {"Sec-WebSocket-Version", "13"},
                    {"Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ=="}
                },
                factory,
                RequestHandlingResult::Kind::Response);

            TEST_ASSERT_NOT_NULL(strstr(responseText.c_str(), "HTTP/1.1 405 Method Not Allowed"));
            TEST_ASSERT_TRUE(factory.createCount() >= 1);
        }

        {
            TestSupport::RecordingRequestHandlerFactory factory(createWebSocketUpgradeHandlerFactory("/chat"));
            const std::string responseText = ExecuteAndCaptureResponseText(
                "GET",
                "/chat",
                {
                    {"Host", "example.test"},
                    {"Upgrade", "websocket"},
                    {"Sec-WebSocket-Version", "13"},
                    {"Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ=="}
                },
                factory,
                RequestHandlingResult::Kind::Response);

            TEST_ASSERT_NOT_NULL(strstr(responseText.c_str(), "HTTP/1.1 426 Upgrade Required"));
            TEST_ASSERT_TRUE(factory.createCount() >= 1);
        }

        {
            TestSupport::RecordingRequestHandlerFactory factory(createWebSocketUpgradeHandlerFactory("/chat"));
            const std::string responseText = ExecuteAndCaptureResponseText(
                "GET",
                "/chat",
                {
                    {"Host", "example.test"},
                    {"Connection", "Upgrade"},
                    {"Sec-WebSocket-Version", "13"},
                    {"Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ=="}
                },
                factory,
                RequestHandlingResult::Kind::Response);

            TEST_ASSERT_NOT_NULL(strstr(responseText.c_str(), "HTTP/1.1 426 Upgrade Required"));
            TEST_ASSERT_TRUE(factory.createCount() >= 1);
        }

        {
            TestSupport::RecordingRequestHandlerFactory factory(createWebSocketUpgradeHandlerFactory("/chat"));
            const std::string responseText = ExecuteAndCaptureResponseText(
                "GET",
                "/chat",
                {
                    {"Host", "example.test"},
                    {"Connection", "Upgrade"},
                    {"Upgrade", "websocket"},
                    {"Sec-WebSocket-Version", "12"},
                    {"Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ=="}
                },
                factory,
                RequestHandlingResult::Kind::Response);

            TEST_ASSERT_NOT_NULL(strstr(responseText.c_str(), "HTTP/1.1 400 Bad Request"));
            TEST_ASSERT_TRUE(factory.createCount() >= 1);
        }

        {
            TestSupport::RecordingRequestHandlerFactory factory(createWebSocketUpgradeHandlerFactory("/chat"));
            const std::string responseText = ExecuteAndCaptureResponseText(
                "GET",
                "/chat",
                {
                    {"Host", "example.test"},
                    {"Connection", "Upgrade"},
                    {"Upgrade", "websocket"},
                    {"Sec-WebSocket-Version", "13"},
                    {"Sec-WebSocket-Key", "short"}
                },
                factory,
                RequestHandlingResult::Kind::Response);

            TEST_ASSERT_NOT_NULL(strstr(responseText.c_str(), "HTTP/1.1 400 Bad Request"));
            TEST_ASSERT_TRUE(factory.createCount() >= 1);
        }

        {
            TestSupport::RecordingRequestHandlerFactory factory(createWebSocketUpgradeHandlerFactory("/chat"));
            const std::string responseText = ExecuteAndCaptureResponseText(
                "GET",
                "/chat",
                {
                    {"Host", "example.test"},
                    {"Connection", "Upgrade"},
                    {"Upgrade", "websocket"},
                    {"Sec-WebSocket-Version", "13"},
                    {"Sec-WebSocket-Key", "!!!!!!!!!!!!!!!!!!!!!!!!"}
                },
                factory,
                RequestHandlingResult::Kind::Response);

            TEST_ASSERT_NOT_NULL(strstr(responseText.c_str(), "HTTP/1.1 400 Bad Request"));
            TEST_ASSERT_TRUE(factory.createCount() >= 1);
        }

        {
            TestSupport::RecordingRequestHandlerFactory factory(createWebSocketUpgradeHandlerFactory("/chat"));
            const std::string responseText = ExecuteAndCaptureResponseText(
                "GET",
                "/chat",
                {
                    {"Host", "example.test"},
                    {"Connection", "Upgrade"},
                    {"Upgrade", "websocket,other"},
                    {"Sec-WebSocket-Version", "13"},
                    {"Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ=="}
                },
                factory,
                RequestHandlingResult::Kind::Response);

            TEST_ASSERT_NOT_NULL(strstr(responseText.c_str(), "HTTP/1.1 400 Bad Request"));
            TEST_ASSERT_TRUE(factory.createCount() >= 1);
        }

        {
            TestSupport::RecordingRequestHandlerFactory factory(createWebSocketUpgradeHandlerFactory("/chat"));
            const std::string responseText = ExecuteAndCaptureResponseText(
                "GET",
                "/chat",
                {
                    {"Host", "example.test"},
                    {"Connection", "Upgrade"},
                    {"Upgrade", "websocket"},
                    {"Sec-WebSocket-Version", "13"},
                    {"Sec-WebSocket-Version", "12"},
                    {"Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ=="}
                },
                factory,
                RequestHandlingResult::Kind::Response);

            TEST_ASSERT_NOT_NULL(strstr(responseText.c_str(), "HTTP/1.1 400 Bad Request"));
            TEST_ASSERT_TRUE(factory.createCount() >= 1);
        }
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_http_context_preserves_custom_method_through_factory_and_handler_steps);
        RUN_TEST(test_http_context_websocket_upgrade_accepts_split_request_and_returns_upgrade_session);
        RUN_TEST(test_http_context_websocket_upgrade_rejects_invalid_requests_with_deterministic_statuses);
        return UNITY_END();
    }
}

int run_test_http_context()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "http request",
        runUnitySuite,
        localSetUp,
        localTearDown);
}