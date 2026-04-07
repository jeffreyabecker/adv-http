#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/httpadv/v1/HttpServerAdvanced.h"

#include <unity.h>

#include "../../src/httpadv/v1/websocket/WebSocketErrorPolicy.h"

using namespace httpadv::v1::core;
using namespace httpadv::v1::handlers;
using namespace httpadv::v1::pipeline;
using namespace httpadv::v1::response;
using namespace httpadv::v1::routing;
using namespace httpadv::v1::server;
using namespace httpadv::v1::staticfiles;
using namespace httpadv::v1::transport;
using namespace lumalink::platform::buffers;
using namespace httpadv::v1::util;
using namespace httpadv::v1::websocket;

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
    return httpadv::v1::TestSupport::RunConsolidatedSuite(
        "websocket error policy",
        runUnitySuite,
        localSetUp,
        localTearDown);
}
