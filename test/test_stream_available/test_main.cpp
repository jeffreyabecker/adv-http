#include "../../test/support/native_unity/unity.h"
#include "../../src/compat/Availability.h"

using namespace HttpServerAdvanced;

// Bridge helper simulating mapping from legacy int available() and a connected flag
static AvailableResult mapLegacyAvailable(int availableValue, bool connected)
{
    AvailableResult r;
    r.count = 0;
    r.errorCode = 0;
    if (availableValue > 0)
    {
        r.state = AvailabilityState::HasBytes;
        r.count = static_cast<std::size_t>(availableValue);
    }
    else if (availableValue == 0)
    {
        if (connected)
            r.state = AvailabilityState::TemporarilyUnavailable;
        else
            r.state = AvailabilityState::Exhausted;
    }
    else // negative
    {
        r.state = AvailabilityState::Error;
        r.errorCode = availableValue;
    }
    return r;
}

void test_mapping_hasbytes()
{
    auto r = mapLegacyAvailable(5, true);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::HasBytes), static_cast<int>(r.state));
    TEST_ASSERT_EQUAL_UINT64(5, r.count);
}

void test_mapping_exhausted()
{
    auto r = mapLegacyAvailable(0, false);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::Exhausted), static_cast<int>(r.state));
}

void test_mapping_temporarily_unavailable()
{
    auto r = mapLegacyAvailable(0, true);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::TemporarilyUnavailable), static_cast<int>(r.state));
}

void test_mapping_error()
{
    auto r = mapLegacyAvailable(-2, true);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::Error), static_cast<int>(r.state));
    TEST_ASSERT_EQUAL_INT(-2, r.errorCode);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_mapping_hasbytes);
    RUN_TEST(test_mapping_exhausted);
    RUN_TEST(test_mapping_temporarily_unavailable);
    RUN_TEST(test_mapping_error);
    return UNITY_END();
}
