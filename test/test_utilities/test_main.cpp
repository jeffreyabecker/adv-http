#include <stdio.h>
#include "../../test/support/native_unity/unity.h"

#include "../../src/util/StringUtility.h"
#include "../../src/util/StringUtility.cpp"

using namespace HttpServerAdvanced::StringUtil;

void setUp() {}
void tearDown() {}

void test_compareTo_basic()
{
    const char* a = "Hello";
    const char* b = "Hello";
    TEST_ASSERT_EQUAL_INT(0, compareTo(a, 5, b, 5, false));
    TEST_ASSERT_EQUAL_INT(-1, compareTo("Abc", 3, "Bcd", 3, false));
}

void test_starts_ends_index()
{
    const char* text = "The quick brown fox";
    TEST_ASSERT_EQUAL_INT(4, indexOf(text, strlen(text), "quick", 5, 0, false));
    TEST_ASSERT_EQUAL_INT(0, compareTo(text, 3, "The", 3, false));
    TEST_ASSERT_EQUAL_INT(16, lastIndexOf(text, strlen(text), "fox", 3, strlen(text)-1, false));
}

void test_replace()
{
    const char* hay = "aabbaabb";
    String replaced = replace(hay, strlen(hay), "bb", 2, "X", 1, false);
    TEST_ASSERT_TRUE(replaced == String("aaXaaX"));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_compareTo_basic);
    RUN_TEST(test_starts_ends_index);
    RUN_TEST(test_replace);
    return UNITY_END();
}
