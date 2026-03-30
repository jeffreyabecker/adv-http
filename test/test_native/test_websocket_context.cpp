#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/HttpServerAdvanced.h"

#include <unity.h>

#include "../../src/websocket\IWebSocketSessionControl.h"
#include "../../src/websocket\WebSocketActivationSnapshot.h"
#include "../../src/websocket\WebSocketContext.h"

#include <any>
#include <string>
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

namespace httpadv::v1::websocket
{
    class WebSocketContextTestProbe
    {
    public:
        static void bind(WebSocketContext &context, IWebSocketSessionControl *control)
        {
            context.bindControl(control);
        }

        static void open(WebSocketContext &context)
        {
            context.notifyOpen();
        }

        static void close(WebSocketContext &context, std::uint16_t closeCode, std::string_view reason)
        {
            context.notifyClose(closeCode, reason);
        }
    };
}

namespace
{
    class FakeSessionControl : public IWebSocketSessionControl
    {
    public:
        WebSocketSendResult nextTextResult = WebSocketSendResult::InvalidState;
        WebSocketSendResult nextBinaryResult = WebSocketSendResult::InvalidState;
        WebSocketCloseResult nextCloseResult = WebSocketCloseResult::InvalidState;

        std::string lastText;
        std::vector<std::uint8_t> lastBinary;
        WebSocketCloseCode lastCloseCode = WebSocketCloseCode::NormalClosure;
        std::string lastCloseReason;

        WebSocketSendResult sendText(std::string_view payload) override
        {
            lastText.assign(payload.data(), payload.size());
            return nextTextResult;
        }

        WebSocketSendResult sendBinary(span<const std::uint8_t> payload) override
        {
            lastBinary.assign(payload.begin(), payload.end());
            return nextBinaryResult;
        }

        WebSocketCloseResult close(WebSocketCloseCode code, std::string_view reason) override
        {
            lastCloseCode = code;
            lastCloseReason.assign(reason.data(), reason.size());
            return nextCloseResult;
        }
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_websocket_context_exposes_activation_snapshot_and_delegates_session_control()
    {
        WebSocketActivationSnapshot snapshot;
        snapshot.items["role"] = std::string("admin");
        snapshot.method = "GET";
        snapshot.version = "1.1";
        snapshot.url = "/chat?room=lobby";
        snapshot.headers.set("X-Trace", "trace-123");
        snapshot.remoteAddress = "10.1.2.3";
        snapshot.remotePort = 4321;
        snapshot.localAddress = "192.168.4.1";
        snapshot.localPort = 8080;

        WebSocketContext context(std::move(snapshot), {});
        FakeSessionControl control;
        control.nextTextResult = WebSocketSendResult::Queued;
        control.nextBinaryResult = WebSocketSendResult::Backpressured;
        control.nextCloseResult = WebSocketCloseResult::CloseQueued;

        WebSocketContextTestProbe::bind(context, &control);
        WebSocketContextTestProbe::open(context);

        TEST_ASSERT_TRUE(context.isOpen());
        TEST_ASSERT_FALSE(context.isClosing());
        TEST_ASSERT_EQUAL_STRING("GET", std::string(context.methodView()).c_str());
        TEST_ASSERT_EQUAL_STRING("1.1", std::string(context.versionView()).c_str());
        TEST_ASSERT_EQUAL_STRING("/chat?room=lobby", std::string(context.urlView()).c_str());
        TEST_ASSERT_EQUAL_STRING("10.1.2.3", std::string(context.remoteAddress()).c_str());
        TEST_ASSERT_EQUAL_UINT16(4321, context.remotePort());
        TEST_ASSERT_EQUAL_STRING("192.168.4.1", std::string(context.localAddress()).c_str());
        TEST_ASSERT_EQUAL_UINT16(8080, context.localPort());
        TEST_ASSERT_EQUAL_STRING("admin", std::any_cast<std::string>(context.items().at("role")).c_str());
        const auto traceHeader = context.headers().find("X-Trace");
        TEST_ASSERT_TRUE(traceHeader.has_value());
        TEST_ASSERT_EQUAL_STRING("trace-123", std::string(traceHeader->valueView()).c_str());

        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketSendResult::Queued), static_cast<int>(context.sendText("hello")));
        TEST_ASSERT_EQUAL_STRING("hello", control.lastText.c_str());

        const std::uint8_t bytes[] = {0x41, 0x42};
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketSendResult::Backpressured), static_cast<int>(context.sendBinary(span<const std::uint8_t>(bytes, sizeof(bytes)))));
        TEST_ASSERT_EQUAL_UINT64(sizeof(bytes), control.lastBinary.size());
        TEST_ASSERT_EQUAL_UINT8('A', control.lastBinary[0]);
        TEST_ASSERT_EQUAL_UINT8('B', control.lastBinary[1]);

        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCloseResult::CloseQueued), static_cast<int>(context.close(WebSocketCloseCode::GoingAway, "later")));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCloseCode::GoingAway), static_cast<int>(control.lastCloseCode));
        TEST_ASSERT_EQUAL_STRING("later", control.lastCloseReason.c_str());
    }

    void test_websocket_context_reports_close_state_when_close_notification_arrives()
    {
        WebSocketContext context(WebSocketActivationSnapshot(), {});
        WebSocketContextTestProbe::close(context, 1000, "bye");

        TEST_ASSERT_FALSE(context.isOpen());
        TEST_ASSERT_FALSE(context.isClosing());
        TEST_ASSERT_TRUE(context.closeCode().has_value());
        TEST_ASSERT_EQUAL_UINT16(1000, *context.closeCode());
        TEST_ASSERT_EQUAL_STRING("bye", std::string(context.closeReason()).c_str());
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketSendResult::InvalidState), static_cast<int>(context.sendText("nope")));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCloseResult::InvalidState), static_cast<int>(context.close()));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_websocket_context_exposes_activation_snapshot_and_delegates_session_control);
        RUN_TEST(test_websocket_context_reports_close_state_when_close_notification_arrives);
        return UNITY_END();
    }
}

int run_test_websocket_context()
{
    return httpadv::v1::TestSupport::RunConsolidatedSuite(
        "websocket context",
        runUnitySuite,
        localSetUp,
        localTearDown);
}