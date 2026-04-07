#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/lumalink/http/LumaLinkHttp.h"

#include <unity.h>

#include "../../src/lumalink/http/util/Clock.h"

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

    void test_manual_clock_starts_at_zero_by_default()
    {
        ManualClock clock;

        TEST_ASSERT_EQUAL_UINT32(0, clock.nowMillis());
    }

    void test_manual_clock_can_advance_and_set_time()
    {
        ManualClock clock(10);

        clock.advanceMillis(15);
        TEST_ASSERT_EQUAL_UINT32(25, clock.nowMillis());

        clock.setNowMillis(7);
        TEST_ASSERT_EQUAL_UINT32(7, clock.nowMillis());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_manual_clock_starts_at_zero_by_default);
        RUN_TEST(test_manual_clock_can_advance_and_set_time);
        return UNITY_END();
    }
}

int run_test_clock()
{
    return lumalink::http::TestSupport::RunConsolidatedSuite(
        "clock",
        runUnitySuite,
        localSetUp,
        localTearDown);
}
