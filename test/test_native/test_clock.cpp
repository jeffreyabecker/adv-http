#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/lumalink/http/LumaLinkHttp.h"

#include <unity.h>

#include <lumalink/platform/time/ManualClock.h>

using namespace lumalink::http::core;
using namespace lumalink::http::handlers;
using namespace lumalink::http::pipeline;
using namespace lumalink::http::response;
using namespace lumalink::http::routing;
using namespace lumalink::http::server;
using namespace lumalink::http::staticfiles;
using namespace lumalink::platform::buffers;
using namespace lumalink::platform::transport;
using namespace lumalink::platform::buffers;
using namespace lumalink::http::util;
using namespace lumalink::http::websocket;
using namespace lumalink::platform::time;

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
        lumalink::platform::time::ManualClock clock;

        TEST_ASSERT_EQUAL_UINT64(0, clock.monotonicNow().value);
    }

    void test_manual_clock_can_advance_and_set_time()
    {
        lumalink::platform::time::ManualClock clock;
        clock.setMonotonicMillis(10);

        clock.advanceMillis(15);
        TEST_ASSERT_EQUAL_UINT64(25, clock.monotonicNow().value);

        clock.setMonotonicMillis(7);
        TEST_ASSERT_EQUAL_UINT64(7, clock.monotonicNow().value);
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
