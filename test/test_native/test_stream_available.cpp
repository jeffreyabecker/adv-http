#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/compat/Availability.h"

using namespace HttpServerAdvanced;

namespace
{
    static AvailableResult mapLegacyAvailable(int availableValue, bool connected)
    {
        AvailableResult result;
        result.count = 0;
        result.errorCode = 0;
        if (availableValue > 0)
        {
            result.state = AvailabilityState::HasBytes;
            result.count = static_cast<std::size_t>(availableValue);
        }
        else if (availableValue == 0)
        {
            result.state = connected ? AvailabilityState::TemporarilyUnavailable : AvailabilityState::Exhausted;
        }
        else
        {
            result.state = AvailabilityState::Error;
            result.errorCode = availableValue;
        }

        return result;
    }

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_mapping_hasbytes()
    {
        auto result = mapLegacyAvailable(5, true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::HasBytes), static_cast<int>(result.state));
        TEST_ASSERT_EQUAL_UINT64(5, result.count);
    }

    void test_mapping_exhausted()
    {
        auto result = mapLegacyAvailable(0, false);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::Exhausted), static_cast<int>(result.state));
    }

    void test_mapping_temporarily_unavailable()
    {
        auto result = mapLegacyAvailable(0, true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::TemporarilyUnavailable), static_cast<int>(result.state));
    }

    void test_mapping_error()
    {
        auto result = mapLegacyAvailable(-2, true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::Error), static_cast<int>(result.state));
        TEST_ASSERT_EQUAL_INT(-2, result.errorCode);
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_mapping_hasbytes);
        RUN_TEST(test_mapping_exhausted);
        RUN_TEST(test_mapping_temporarily_unavailable);
        RUN_TEST(test_mapping_error);
        return UNITY_END();
    }
}

int run_test_stream_available()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "stream available",
        runUnitySuite,
        localSetUp,
        localTearDown);
}