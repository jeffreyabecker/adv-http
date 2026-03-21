#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

namespace
{
    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_native_unity_harness_runs()
    {
        TEST_ASSERT_TRUE(true);
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_native_unity_harness_runs);
        return UNITY_END();
    }
}

int run_test_native_smoke()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "native smoke",
        runUnitySuite,
        localSetUp,
        localTearDown);
}