#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/compat/Clock.h"

using namespace HttpServerAdvanced;

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
        Compat::ManualClock clock;

        TEST_ASSERT_EQUAL_UINT32(0, clock.nowMillis());
    }

    void test_manual_clock_can_advance_and_set_time()
    {
        Compat::ManualClock clock(10);

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
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "clock",
        runUnitySuite,
        localSetUp,
        localTearDown);
}