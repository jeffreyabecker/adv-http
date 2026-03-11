#include <unity.h>

void setUp()
{
}

void tearDown()
{
}

namespace
{
    void test_native_unity_harness_runs()
    {
        TEST_ASSERT_TRUE(true);
    }
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_native_unity_harness_runs);
    return UNITY_END();
}