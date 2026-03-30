#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/HttpServerAdvanced.h"

#include <unity.h>

#include "../../src/compat/Clock.h"

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
    return httpadv::v1::TestSupport::RunConsolidatedSuite(
        "clock",
        runUnitySuite,
        localSetUp,
        localTearDown);
}