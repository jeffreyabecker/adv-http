#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/lumalink/http/HttpServerAdvanced.h"

#include <unity.h>

#include "../../src/lumalink/http/websocket/WebSocketErrorPolicy.h"

using namespace lumalink::http::core;
using namespace lumalink::http::handlers;
using namespace lumalink::http::pipeline;
using namespace lumalink::http::response;
using namespace lumalink::http::routing;
using namespace lumalink::http::server;
using namespace lumalink::http::staticfiles;
using namespace lumalink::http::transport;
using namespace lumalink::platform::buffers;
using namespace lumalink::http::util;
using namespace lumalink::http::websocket;

namespace
{
    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_websocket_error_policy_table_matches_authoritative_mapping()
    {
        {
            constexpr WsClosePolicy policy = policyFor(WsErrorCategory::FrameParseError);
            TEST_ASSERT_FALSE(policy.attemptCloseHandshake);
            TEST_ASSERT_EQUAL_UINT16(1002, policy.closeCode);
        }

        {
            constexpr WsClosePolicy policy = policyFor(WsErrorCategory::ProtocolViolation);
            TEST_ASSERT_TRUE(policy.attemptCloseHandshake);
            TEST_ASSERT_EQUAL_UINT16(1002, policy.closeCode);
        }

        {
            constexpr WsClosePolicy policy = policyFor(WsErrorCategory::MessageTooLarge);
            TEST_ASSERT_TRUE(policy.attemptCloseHandshake);
            TEST_ASSERT_EQUAL_UINT16(1009, policy.closeCode);
        }

        {
            constexpr WsClosePolicy policy = policyFor(WsErrorCategory::WriteFailure);
            TEST_ASSERT_FALSE(policy.attemptCloseHandshake);
            TEST_ASSERT_EQUAL_UINT16(1006, policy.closeCode);
        }

        {
            constexpr WsClosePolicy policy = policyFor(WsErrorCategory::IdleTimeout);
            TEST_ASSERT_TRUE(policy.attemptCloseHandshake);
            TEST_ASSERT_EQUAL_UINT16(1001, policy.closeCode);
        }

        {
            constexpr WsClosePolicy policy = policyFor(WsErrorCategory::CloseHandshakeTimeout);
            TEST_ASSERT_FALSE(policy.attemptCloseHandshake);
            TEST_ASSERT_EQUAL_UINT16(1006, policy.closeCode);
        }

        {
            constexpr WsClosePolicy policy = policyFor(WsErrorCategory::RemoteDisconnect);
            TEST_ASSERT_FALSE(policy.attemptCloseHandshake);
            TEST_ASSERT_EQUAL_UINT16(1006, policy.closeCode);
        }
    }

    void test_websocket_error_policy_returns_safe_default_for_unmapped_category()
    {
        const WsClosePolicy policy = policyFor(static_cast<WsErrorCategory>(999));
        TEST_ASSERT_FALSE(policy.attemptCloseHandshake);
        TEST_ASSERT_EQUAL_UINT16(1006, policy.closeCode);
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_websocket_error_policy_table_matches_authoritative_mapping);
        RUN_TEST(test_websocket_error_policy_returns_safe_default_for_unmapped_category);
        return UNITY_END();
    }
}

int run_test_websocket_error_policy()
{
    return lumalink::http::TestSupport::RunConsolidatedSuite(
        "websocket error policy",
        runUnitySuite,
        localSetUp,
        localTearDown);
}
