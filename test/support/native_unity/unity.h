// Minimal Unity-like test helpers for native runs in this repo.
// Provides a tiny subset of Unity macros used by existing tests.
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int UNITY_FAIL_COUNT = 0;
static int UNITY_TEST_COUNT = 0;

#define UNITY_BEGIN() (UNITY_FAIL_COUNT = 0, UNITY_TEST_COUNT = 0, 0)

inline void unity_run_test_impl(void (*test)(), const char* name) {
    ++UNITY_TEST_COUNT;
    printf("RUNNING: %s\n", name);
    test();
}

#define RUN_TEST(fn) unity_run_test_impl(fn, #fn)

inline int UNITY_END() {
    if (UNITY_FAIL_COUNT == 0) {
        printf("\nTESTS PASSED (%d)\n", UNITY_TEST_COUNT);
        return 0;
    } else {
        printf("\nTESTS FAILED: %d of %d\n", UNITY_FAIL_COUNT, UNITY_TEST_COUNT);
        return 1;
    }
}

#define UNITY_FAIL_AND_BAIL(msg) do { \
    ++UNITY_FAIL_COUNT; \
    printf("ASSERT FAILED: %s\n", msg); \
    return; \
} while(0)

#define TEST_ASSERT_TRUE(cond) do { if (!(cond)) UNITY_FAIL_AND_BAIL(#cond); } while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual) do { if ((int)(expected) != (int)(actual)) { \
    printf("ASSERT EQUAL INT FAILED: expected=%d actual=%d\n", (int)(expected), (int)(actual)); \
    UNITY_FAIL_AND_BAIL("TEST_ASSERT_EQUAL_INT"); } } while(0)

#define TEST_ASSERT_EQUAL_UINT64(expected, actual) do { if ((unsigned long long)(expected) != (unsigned long long)(actual)) { \
    printf("ASSERT EQUAL UINT64 FAILED: expected=%llu actual=%llu\n", (unsigned long long)(expected), (unsigned long long)(actual)); \
    UNITY_FAIL_AND_BAIL("TEST_ASSERT_EQUAL_UINT64"); } } while(0)

#define TEST_ASSERT_EQUAL_UINT32(expected, actual) TEST_ASSERT_EQUAL_UINT64(expected, actual)
#define TEST_ASSERT_EQUAL_UINT8(expected, actual) do { if ((unsigned)(expected) != (unsigned)(actual)) { \
    printf("ASSERT EQUAL UINT8 FAILED: expected=%u actual=%u\n", (unsigned)(expected), (unsigned)(actual)); \
    UNITY_FAIL_AND_BAIL("TEST_ASSERT_EQUAL_UINT8"); } } while(0)
