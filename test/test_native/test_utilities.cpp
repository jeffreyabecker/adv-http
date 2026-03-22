#include "../support/include/ConsolidatedNativeSuite.h"

#include <Arduino.h>
#include <cstring>
#include <unity.h>

#include "../../src/util/HttpUtility.h"
#include "../../src/util/StringUtility.h"
#include "../../src/core/HttpHeaderCollection.h"
#include "../../src/util/UriView.h"

using namespace HttpServerAdvanced::StringUtil;

namespace
{
    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_compareTo_basic()
    {
        const char *a = "Hello";
        const char *b = "Hello";
        TEST_ASSERT_EQUAL_INT(0, compareTo(a, 5, b, 5, false));
        TEST_ASSERT_EQUAL_INT(-1, compareTo("Abc", 3, "Bcd", 3, false));
    }

    void test_starts_ends_index()
    {
        const char *text = "The quick brown fox";
        TEST_ASSERT_EQUAL_INT(4, indexOf(text, strlen(text), "quick", 5, 0, false));
        TEST_ASSERT_EQUAL_INT(0, compareTo(text, 3, "The", 3, false));
        TEST_ASSERT_EQUAL_INT(16, lastIndexOf(text, strlen(text), "fox", 3, strlen(text) - 1, false));
    }

    void test_replace()
    {
        const char *hay = "aabbaabb";
        const std::string replaced = replace(hay, strlen(hay), "bb", 2, "X", 1, false);
        TEST_ASSERT_EQUAL_STRING("aaXaaX", replaced.c_str());
    }

    void test_string_utility_string_view_overloads()
    {
        const std::string_view haystack = "AlphaBetaGamma";
        TEST_ASSERT_EQUAL_INT(0, compareTo(std::string_view("abc"), std::string_view("ABC"), true));
        TEST_ASSERT_TRUE(startsWith(haystack, std::string_view("Alpha"), false));
        TEST_ASSERT_TRUE(endsWith(haystack, std::string_view("Gamma"), false));
        TEST_ASSERT_EQUAL_INT(5, indexOf(haystack, std::string_view("Beta"), 0, false));
        TEST_ASSERT_EQUAL_INT(13, lastIndexOf(haystack, std::string_view("a"), haystack.size() - 1, true));
    }

    void test_parse_query_parameters_uses_std_string_payloads()
    {
        const auto params = HttpServerAdvanced::WebUtility::ParseQueryParameters("name=Jane+Doe&role=admin&empty", strlen("name=Jane+Doe&role=admin&empty"));

        TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(params.pairs().size()));
        const auto name = params.get("name");
        TEST_ASSERT_TRUE(name.has_value());
        TEST_ASSERT_EQUAL_STRING("Jane Doe", name->c_str());

        const auto role = params.get(std::string("role"));
        TEST_ASSERT_TRUE(role.has_value());
        TEST_ASSERT_EQUAL_STRING("admin", role->c_str());

        const auto empty = params.get("empty");
        TEST_ASSERT_TRUE(empty.has_value());
        TEST_ASSERT_TRUE(empty->empty());
    }

    void test_web_utility_std_string_view_overloads_remain_available()
    {
        const std::string_view query = "first=one&second=two";
        const auto pairs = HttpServerAdvanced::WebUtility::ParseQueryString(query);

        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(pairs.size()));
        TEST_ASSERT_TRUE(pairs[0].first == String("first"));
        TEST_ASSERT_TRUE(pairs[0].second == String("one"));

        const String encoded = HttpServerAdvanced::WebUtility::HtmlEncode(std::string_view("<&>\"'"));
        TEST_ASSERT_TRUE(encoded == String("&lt;&amp;&gt;&quot;&#39;"));

        const String decoded = HttpServerAdvanced::WebUtility::DecodeURIComponent(std::string_view("Jane%20Doe"));
        TEST_ASSERT_TRUE(decoded == String("Jane Doe"));
    }

    void test_uri_view_exposes_string_view_segments_and_query_payload()
    {
        const HttpServerAdvanced::UriView uri("https://user@example.com:8443/path/to/resource?first=one&second=two#frag");

        TEST_ASSERT_EQUAL_STRING("https", std::string(uri.scheme()).c_str());
        TEST_ASSERT_EQUAL_STRING("user", std::string(uri.userinfo()).c_str());
        TEST_ASSERT_EQUAL_STRING("example.com", std::string(uri.host()).c_str());
        TEST_ASSERT_EQUAL_STRING("8443", std::string(uri.port()).c_str());
        TEST_ASSERT_EQUAL_STRING("/path/to/resource", std::string(uri.path()).c_str());
        TEST_ASSERT_EQUAL_STRING("first=one&second=two", std::string(uri.query()).c_str());
        TEST_ASSERT_EQUAL_STRING("frag", std::string(uri.fragment()).c_str());

        const auto first = uri.queryView().get("first");
        TEST_ASSERT_TRUE(first.has_value());
        TEST_ASSERT_EQUAL_STRING("one", first->c_str());
    }

    void test_header_collection_supports_string_view_lookups()
    {
        HttpServerAdvanced::HttpHeaderCollection headers;
        headers.set("Content-Type", "application/json");
        headers.set("X-Test", "first", false);
        headers.set("X-Test", "second", false);

        const auto contentType = headers.find(std::string_view("content-type"));
        TEST_ASSERT_TRUE(contentType.has_value());
        TEST_ASSERT_EQUAL_STRING("Content-Type", std::string(contentType->nameView()).c_str());
        TEST_ASSERT_EQUAL_STRING("application/json", std::string(contentType->valueView()).c_str());

        TEST_ASSERT_TRUE(headers.exists(std::string_view("x-test")));
        TEST_ASSERT_TRUE(headers.exists(std::string_view("x-test"), std::string_view("first,second")));

        headers.remove(std::string_view("content-type"));
        TEST_ASSERT_FALSE(headers.exists(std::string_view("content-type")));
    }

    void test_http_header_preserves_standard_text_storage_with_arduino_adapters()
    {
        const HttpServerAdvanced::HttpHeader header(std::string("X-Mode"), std::string("native"));

        TEST_ASSERT_EQUAL_STRING("X-Mode", std::string(header.nameView()).c_str());
        TEST_ASSERT_EQUAL_STRING("native", std::string(header.valueView()).c_str());
        TEST_ASSERT_TRUE(header.name() == String("X-Mode"));
        TEST_ASSERT_TRUE(header.value() == String("native"));
    }

    void test_html_encode_preserves_expected_entities_without_progmem_helpers()
    {
        const String encoded = HttpServerAdvanced::WebUtility::HtmlEncode("<&>\"'", 5);

        TEST_ASSERT_TRUE(encoded == String("&lt;&amp;&gt;&quot;&#39;"));
    }

    void test_html_attribute_encode_preserves_expected_entities_without_progmem_helpers()
    {
        const String encoded = HttpServerAdvanced::WebUtility::HtmlAttributeEncode("<&>\"'", 5);

        TEST_ASSERT_TRUE(encoded == String("&lt;&amp;&gt;&quot;&#39;"));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_compareTo_basic);
        RUN_TEST(test_starts_ends_index);
        RUN_TEST(test_replace);
        RUN_TEST(test_string_utility_string_view_overloads);
        RUN_TEST(test_parse_query_parameters_uses_std_string_payloads);
        RUN_TEST(test_web_utility_std_string_view_overloads_remain_available);
        RUN_TEST(test_uri_view_exposes_string_view_segments_and_query_payload);
        RUN_TEST(test_header_collection_supports_string_view_lookups);
        RUN_TEST(test_http_header_preserves_standard_text_storage_with_arduino_adapters);
        RUN_TEST(test_html_encode_preserves_expected_entities_without_progmem_helpers);
        RUN_TEST(test_html_attribute_encode_preserves_expected_entities_without_progmem_helpers);
        return UNITY_END();
    }
}

int run_test_utilities()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "utilities",
        runUnitySuite,
        localSetUp,
        localTearDown);
}