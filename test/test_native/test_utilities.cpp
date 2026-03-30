#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <any>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <unity.h>
#include <utility>
#include <vector>

#include "../../src/compat/platform/windows/VirtualPathMapperWindows.h"
#include "../../src/compat/platform/posix/VirtualPathMapperPosix.h"
#include "../../src/core/HttpContentTypes.h"
#include "../../src/core/HttpMethod.h"
#include "../../src/core/HttpStatus.h"
#include "../../src/core/IHttpContextHandlerFactory.h"
#include "../../src/core/Defines.h"
#include "../../src/core/HttpContext.h"
#include "../../src/util/HttpUtility.h"
#include "../../src/core/HttpHeaderCollection.h"
#include "../../src/routing/CrossOriginRequestSharing.h"
#include "../../src/routing/HandlerMatcher.h"
#include "../../src/response/StringResponse.h"
#include "../../src/util/UriView.h"

namespace
{
    void localSetUp()
    {
    }

    void localTearDown()
    {
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
        TEST_ASSERT_EQUAL_STRING("first", pairs[0].first.c_str());
        TEST_ASSERT_EQUAL_STRING("one", pairs[0].second.c_str());

        const std::string encoded = HttpServerAdvanced::WebUtility::HtmlEncode(std::string_view("<&>\"'"));
        TEST_ASSERT_EQUAL_STRING("&lt;&amp;&gt;&quot;&#39;", encoded.c_str());

        const std::string decoded = HttpServerAdvanced::WebUtility::DecodeURIComponent(std::string_view("Jane%20Doe"));
        TEST_ASSERT_EQUAL_STRING("Jane Doe", decoded.c_str());
    }

    void test_web_utility_base64_decode_to_std_string()
    {
        const std::string decoded = HttpServerAdvanced::WebUtility::Base64DecodeToString(std::string_view("SmFuZSBEb2U="));

        TEST_ASSERT_EQUAL_STRING("Jane Doe", decoded.c_str());
    }

    void test_web_utility_base64_decode_invalid_input_returns_empty_output()
    {
        const std::vector<uint8_t> decoded = HttpServerAdvanced::WebUtility::Base64Decode(std::string_view("!!!!"));

        TEST_ASSERT_TRUE(decoded.empty());
    }

    void test_web_utility_uri_encoding_preserves_current_percent_encoding_behavior()
    {
        const std::string encodedFromView = HttpServerAdvanced::WebUtility::EncodeURIComponent(std::string_view("one two/three?four=five&six"));
        const std::string encodedFromChars = HttpServerAdvanced::WebUtility::EncodeURIComponent("one two/three?four=five&six", strlen("one two/three?four=five&six"));

        TEST_ASSERT_EQUAL_STRING("one%20two%2Fthree%3Ffour%3Dfive%26six", encodedFromView.c_str());
        TEST_ASSERT_EQUAL_STRING(encodedFromView.c_str(), encodedFromChars.c_str());
    }

    void test_web_utility_decode_uri_component_freezes_legacy_malformed_percent_handling()
    {
        const std::string validDecoded = HttpServerAdvanced::WebUtility::DecodeURIComponent(std::string_view("alpha%2Fbeta+gamma"));
        const std::string malformedHexDecoded = HttpServerAdvanced::WebUtility::DecodeURIComponent(std::string_view("%ZZ"));
        const std::string partialPercentDecoded = HttpServerAdvanced::WebUtility::DecodeURIComponent(std::string_view("trail%"));
        const std::string partialNibbleDecoded = HttpServerAdvanced::WebUtility::DecodeURIComponent(std::string_view("trail%A"));

        TEST_ASSERT_EQUAL_STRING("alpha/beta gamma", validDecoded.c_str());

        TEST_ASSERT_EQUAL_UINT64(1, malformedHexDecoded.size());
        TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(malformedHexDecoded[0]));

        TEST_ASSERT_EQUAL_STRING("trail", partialPercentDecoded.c_str());
        TEST_ASSERT_EQUAL_STRING("trail", partialNibbleDecoded.c_str());
    }

    void test_parse_query_parameters_freezes_empty_repeated_and_plus_decoded_values()
    {
        const std::string_view query = "=value&&name=&name=second&plus=one+two&trailing=";
        const auto params = HttpServerAdvanced::WebUtility::ParseQueryParameters(query);

        TEST_ASSERT_EQUAL_UINT32(6, static_cast<uint32_t>(params.pairs().size()));
        TEST_ASSERT_EQUAL_UINT64(2, params.exists("name"));
        TEST_ASSERT_EQUAL_UINT64(2, params.exists(""));

        const auto emptyKey = params.get("");
        TEST_ASSERT_TRUE(emptyKey.has_value());
        TEST_ASSERT_EQUAL_STRING("value", emptyKey->c_str());

        const auto firstName = params.get("name");
        TEST_ASSERT_TRUE(firstName.has_value());
        TEST_ASSERT_EQUAL_STRING("", firstName->c_str());

        const auto allNames = params.getAll("name");
        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(allNames.size()));
        TEST_ASSERT_EQUAL_STRING("", allNames[0].c_str());
        TEST_ASSERT_EQUAL_STRING("second", allNames[1].c_str());

        const auto plus = params.get("plus");
        TEST_ASSERT_TRUE(plus.has_value());
        TEST_ASSERT_EQUAL_STRING("one two", plus->c_str());

        const auto trailing = params.get("trailing");
        TEST_ASSERT_TRUE(trailing.has_value());
        TEST_ASSERT_TRUE(trailing->empty());
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

    void test_uri_view_handles_relative_query_only_fragment_only_and_authority_less_uris()
    {
        const HttpServerAdvanced::UriView relativeUri("/docs/page?mode=edit#section-2");
        TEST_ASSERT_TRUE(relativeUri.scheme().empty());
        TEST_ASSERT_TRUE(relativeUri.host().empty());
        TEST_ASSERT_EQUAL_STRING("/docs/page", std::string(relativeUri.path()).c_str());
        TEST_ASSERT_EQUAL_STRING("mode=edit", std::string(relativeUri.query()).c_str());
        TEST_ASSERT_EQUAL_STRING("section-2", std::string(relativeUri.fragment()).c_str());

        const HttpServerAdvanced::UriView queryOnlyUri("?first=one&first=two");
        TEST_ASSERT_TRUE(queryOnlyUri.path().empty());
        TEST_ASSERT_EQUAL_STRING("first=one&first=two", std::string(queryOnlyUri.query()).c_str());
        TEST_ASSERT_EQUAL_UINT64(2, queryOnlyUri.queryView().exists("first"));

        const HttpServerAdvanced::UriView fragmentOnlyUri("#frag-only");
        TEST_ASSERT_TRUE(fragmentOnlyUri.path().empty());
        TEST_ASSERT_TRUE(fragmentOnlyUri.query().empty());
        TEST_ASSERT_EQUAL_STRING("frag-only", std::string(fragmentOnlyUri.fragment()).c_str());

        const HttpServerAdvanced::UriView authorityLessUri("file:/tmp/data.json?download=true#anchor");
        TEST_ASSERT_EQUAL_STRING("file", std::string(authorityLessUri.scheme()).c_str());
        TEST_ASSERT_TRUE(authorityLessUri.host().empty());
        TEST_ASSERT_EQUAL_STRING("/tmp/data.json", std::string(authorityLessUri.path()).c_str());
        TEST_ASSERT_EQUAL_STRING("download=true", std::string(authorityLessUri.query()).c_str());
        TEST_ASSERT_EQUAL_STRING("anchor", std::string(authorityLessUri.fragment()).c_str());
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

    void test_header_collection_freezes_merge_overwrite_and_set_cookie_semantics()
    {
        HttpServerAdvanced::HttpHeaderCollection headers;
        headers.set("X-Test", "first");
        headers.set("X-Test", "second", false);

        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(headers.size()));
        TEST_ASSERT_TRUE(headers.exists("x-test", "first,second"));

        headers.set("X-Test", "replacement", true);
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(headers.size()));
        TEST_ASSERT_TRUE(headers.exists("x-test", "replacement"));
        TEST_ASSERT_FALSE(headers.exists("x-test", "first,second"));

        headers.set(HttpServerAdvanced::HttpHeader::SetCookie("a=1"), false);
        headers.set(HttpServerAdvanced::HttpHeader::SetCookie("b=2"), false);

        TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(headers.size()));
        TEST_ASSERT_TRUE(headers.exists("set-cookie", "a=1"));
        TEST_ASSERT_TRUE(headers.exists("set-cookie", "b=2"));

        headers.remove("set-cookie", "a=1");
        TEST_ASSERT_FALSE(headers.exists("set-cookie", "a=1"));
        TEST_ASSERT_TRUE(headers.exists("set-cookie", "b=2"));

        headers.remove("set-cookie");
        TEST_ASSERT_FALSE(headers.exists("set-cookie"));
    }

    void test_header_collection_preserves_insertion_slot_and_negative_lookup_behavior()
    {
        HttpServerAdvanced::HttpHeaderCollection headers;
        headers.set("X-First", "one");
        headers.set("X-Second", "two");
        headers.set("X-First", "merged", false);
        headers.set(HttpServerAdvanced::HttpHeader::SetCookie("a=1"), false);
        headers.set(HttpServerAdvanced::HttpHeader::SetCookie("b=2"), false);

        TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(headers.size()));
        TEST_ASSERT_EQUAL_STRING("X-First", std::string(headers[0].nameView()).c_str());
        TEST_ASSERT_EQUAL_STRING("one,merged", std::string(headers[0].valueView()).c_str());
        TEST_ASSERT_EQUAL_STRING("X-Second", std::string(headers[1].nameView()).c_str());
        TEST_ASSERT_EQUAL_STRING("Set-Cookie", std::string(headers[2].nameView()).c_str());
        TEST_ASSERT_EQUAL_STRING("a=1", std::string(headers[2].valueView()).c_str());
        TEST_ASSERT_EQUAL_STRING("b=2", std::string(headers[3].valueView()).c_str());

        TEST_ASSERT_FALSE(headers.exists("missing-header"));
        TEST_ASSERT_FALSE(headers.exists("x-first", "wrong"));
        TEST_ASSERT_FALSE(headers.find("missing-header").has_value());
    }

    void test_http_header_preserves_standard_text_storage()
    {
        const HttpServerAdvanced::HttpHeader header(std::string("X-Mode"), std::string("native"));

        TEST_ASSERT_EQUAL_STRING("X-Mode", std::string(header.nameView()).c_str());
        TEST_ASSERT_EQUAL_STRING("native", std::string(header.valueView()).c_str());
        TEST_ASSERT_EQUAL_STRING("X-Mode", std::string(header.name()).c_str());
        TEST_ASSERT_EQUAL_STRING("native", std::string(header.value()).c_str());
    }

    void test_http_header_supports_empty_copy_and_move_semantics()
    {
        const HttpServerAdvanced::HttpHeader emptyHeader{std::string(), std::string()};
        TEST_ASSERT_TRUE(emptyHeader.nameView().empty());
        TEST_ASSERT_TRUE(emptyHeader.valueView().empty());

        const HttpServerAdvanced::HttpHeader copiedHeader(emptyHeader);
        TEST_ASSERT_TRUE(copiedHeader.nameView().empty());
        TEST_ASSERT_TRUE(copiedHeader.valueView().empty());

        HttpServerAdvanced::HttpHeader movedHeader(HttpServerAdvanced::HttpHeader("X-Trace", "abc123"));
        TEST_ASSERT_EQUAL_STRING("X-Trace", std::string(movedHeader.nameView()).c_str());
        TEST_ASSERT_EQUAL_STRING("abc123", std::string(movedHeader.valueView()).c_str());
    }

    void test_http_status_exposes_code_comparison_and_description_behavior()
    {
        const HttpServerAdvanced::HttpStatus ok = HttpServerAdvanced::HttpStatus::Ok();
        const HttpServerAdvanced::HttpStatus custom = HttpServerAdvanced::HttpStatus(299, "Custom Success");

        TEST_ASSERT_EQUAL_UINT16(200, ok.code());
        TEST_ASSERT_EQUAL_STRING("OK", ok.toString());
        TEST_ASSERT_TRUE(ok == static_cast<uint16_t>(200));
        TEST_ASSERT_TRUE(HttpServerAdvanced::HttpStatus::BadRequest() < HttpServerAdvanced::HttpStatus::InternalServerError());
        TEST_ASSERT_EQUAL_STRING("Custom Success", custom.toString());
    }

    void test_http_method_request_body_allowance_matches_current_policy()
    {
        TEST_ASSERT_TRUE(HttpServerAdvanced::HttpMethod::isRequestBodyAllowed(HttpServerAdvanced::HttpMethod::Post));
        TEST_ASSERT_TRUE(HttpServerAdvanced::HttpMethod::isRequestBodyAllowed(HttpServerAdvanced::HttpMethod::Put));
        TEST_ASSERT_TRUE(HttpServerAdvanced::HttpMethod::isRequestBodyAllowed(HttpServerAdvanced::HttpMethod::Patch));
        TEST_ASSERT_TRUE(HttpServerAdvanced::HttpMethod::isRequestBodyAllowed(HttpServerAdvanced::HttpMethod::Delete));

        TEST_ASSERT_FALSE(HttpServerAdvanced::HttpMethod::isRequestBodyAllowed(HttpServerAdvanced::HttpMethod::Get));
        TEST_ASSERT_FALSE(HttpServerAdvanced::HttpMethod::isRequestBodyAllowed(HttpServerAdvanced::HttpMethod::Head));
        TEST_ASSERT_FALSE(HttpServerAdvanced::HttpMethod::isRequestBodyAllowed(HttpServerAdvanced::HttpMethod::Options));
        TEST_ASSERT_FALSE(HttpServerAdvanced::HttpMethod::isRequestBodyAllowed("CUSTOM"));
    }

    void test_http_content_types_normalizes_extensions_and_falls_back_to_unknown()
    {
        HttpServerAdvanced::HttpContentTypes contentTypes;

        TEST_ASSERT_EQUAL_STRING(HttpServerAdvanced::HttpContentTypes::Json, contentTypes.getContentTypeByExtension("JSON"));
        TEST_ASSERT_EQUAL_STRING(HttpServerAdvanced::HttpContentTypes::Svg, contentTypes.getContentTypeFromPath("/assets/icon.SVG"));
        TEST_ASSERT_EQUAL_STRING(HttpServerAdvanced::HttpContentTypes::Unknown, contentTypes.getContentTypeFromPath("/assets/README"));

        contentTypes.set("MD", "text/markdown");
        TEST_ASSERT_EQUAL_STRING("text/markdown", contentTypes.getContentTypeByExtension("md"));
    }

    void test_virtual_path_mapper_windows_normalizes_segments_and_names()
    {
        using HttpServerAdvanced::Compat::VirtualPathMapperWindows;

        TEST_ASSERT_TRUE(VirtualPathMapperWindows::HasDriveLetterPrefix("C:/nested/asset.txt"));
        TEST_ASSERT_FALSE(VirtualPathMapperWindows::HasDriveLetterPrefix("/nested/asset.txt"));

        TEST_ASSERT_EQUAL_STRING("nested/asset.txt", VirtualPathMapperWindows::NormalizePath("C:\\nested\\.\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("outside.txt", VirtualPathMapperWindows::NormalizePath("../outside.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("/nested/asset.txt", VirtualPathMapperWindows::WithLeadingSlash("nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("/nested/asset.txt", VirtualPathMapperWindows::Join("/nested", "asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("asset.txt", std::string(VirtualPathMapperWindows::BasenameView("/nested/asset.txt")).c_str());
        TEST_ASSERT_EQUAL_STRING("C:\\nested\\asset.txt", VirtualPathMapperWindows::NormalizeScopedPath("C:/nested/asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("C:\\nested\\asset.txt", VirtualPathMapperWindows::JoinScopedPath("C:\\nested", "asset.txt").c_str());
    }

    void test_virtual_path_mapper_windows_constructs_from_chroot_spec()
    {
        using HttpServerAdvanced::Compat::VirtualPathMapperWindows;

        const VirtualPathMapperWindows unrooted;
        TEST_ASSERT_FALSE(unrooted.hasRootPath());
        TEST_ASSERT_TRUE(unrooted.rootPath().empty());
        TEST_ASSERT_EQUAL_STRING("nested\\asset.txt", unrooted.exposePath("nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("nested\\asset.txt", unrooted.resolveScopedPath("nested\\asset.txt").c_str());

        const VirtualPathMapperWindows rooted("assets/");
        TEST_ASSERT_TRUE(rooted.hasRootPath());
        TEST_ASSERT_EQUAL_STRING("assets", rooted.rootPath().c_str());
        TEST_ASSERT_EQUAL_STRING("/nested/asset.txt", rooted.exposePath("nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("assets\\nested\\asset.txt", rooted.resolveScopedPath("nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("assets", rooted.resolveScopedPath(".").c_str());
    }

    void test_virtual_path_mapper_posix_normalizes_segments_and_names()
    {
        using HttpServerAdvanced::Compat::VirtualPathMapperPosix;

        TEST_ASSERT_TRUE(VirtualPathMapperPosix::HasDriveLetterPrefix("C:/nested/asset.txt"));
        TEST_ASSERT_FALSE(VirtualPathMapperPosix::HasDriveLetterPrefix("/nested/asset.txt"));

        TEST_ASSERT_EQUAL_STRING("nested/asset.txt", VirtualPathMapperPosix::NormalizePath("C:\\nested\\.\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("outside.txt", VirtualPathMapperPosix::NormalizePath("../outside.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("/nested/asset.txt", VirtualPathMapperPosix::WithLeadingSlash("nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("/nested/asset.txt", VirtualPathMapperPosix::Join("/nested", "asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("asset.txt", std::string(VirtualPathMapperPosix::BasenameView("/nested/asset.txt")).c_str());
        TEST_ASSERT_EQUAL_STRING("C:/nested/asset.txt", VirtualPathMapperPosix::NormalizeScopedPath("C:\\nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("/nested/asset.txt", VirtualPathMapperPosix::JoinScopedPath("/nested", "asset.txt").c_str());
    }

    void test_virtual_path_mapper_posix_constructs_from_chroot_spec()
    {
        using HttpServerAdvanced::Compat::VirtualPathMapperPosix;

        const VirtualPathMapperPosix unrooted;
        TEST_ASSERT_FALSE(unrooted.hasRootPath());
        TEST_ASSERT_TRUE(unrooted.rootPath().empty());
        TEST_ASSERT_EQUAL_STRING("nested\\asset.txt", unrooted.exposePath("nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("nested/asset.txt", unrooted.resolveScopedPath("nested\\asset.txt").c_str());

        const VirtualPathMapperPosix rooted("assets/");
        TEST_ASSERT_TRUE(rooted.hasRootPath());
        TEST_ASSERT_EQUAL_STRING("assets", rooted.rootPath().c_str());
        TEST_ASSERT_EQUAL_STRING("/nested/asset.txt", rooted.exposePath("nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("assets/nested/asset.txt", rooted.resolveScopedPath("nested\\asset.txt").c_str());
        TEST_ASSERT_EQUAL_STRING("assets", rooted.resolveScopedPath(".").c_str());
    }

    void test_html_encode_returns_empty_for_null_or_empty_inputs()
    {
        const std::string encodedNull = HttpServerAdvanced::WebUtility::HtmlEncode(static_cast<const char *>(nullptr));
        const std::string encodedEmpty = HttpServerAdvanced::WebUtility::HtmlEncode("", 0);

        TEST_ASSERT_TRUE(encodedNull.empty());
        TEST_ASSERT_TRUE(encodedEmpty.empty());
    }

    void test_html_encode_preserves_expected_entities_without_progmem_helpers()
    {
        const std::string encoded = HttpServerAdvanced::WebUtility::HtmlEncode("<&>\"'", 5);

        TEST_ASSERT_EQUAL_STRING("&lt;&amp;&gt;&quot;&#39;", encoded.c_str());
    }

    void test_html_attribute_encode_returns_empty_for_null_or_empty_inputs()
    {
        const std::string encodedNull = HttpServerAdvanced::WebUtility::HtmlAttributeEncode(static_cast<const char *>(nullptr));
        const std::string encodedEmpty = HttpServerAdvanced::WebUtility::HtmlAttributeEncode("", 0);

        TEST_ASSERT_TRUE(encodedNull.empty());
        TEST_ASSERT_TRUE(encodedEmpty.empty());
    }

    void test_html_attribute_encode_preserves_expected_entities_without_progmem_helpers()
    {
        const std::string encoded = HttpServerAdvanced::WebUtility::HtmlAttributeEncode("<&>\"'", 5);

        TEST_ASSERT_EQUAL_STRING("&lt;&amp;&gt;&quot;&#39;", encoded.c_str());
    }

    void test_javascript_string_encode_escapes_control_characters_and_quotes()
    {
        const std::string encodedWithQuotes = HttpServerAdvanced::WebUtility::JavaScriptStringEncode(std::string_view("\"\\\n\t\x01"));
        const std::string encodedWithoutQuotes = HttpServerAdvanced::WebUtility::JavaScriptStringEncode(std::string_view("plain"), false);
        const std::string encodedEmptyWithQuotes = HttpServerAdvanced::WebUtility::JavaScriptStringEncode(std::string_view(), true);
        const std::string encodedEmptyWithoutQuotes = HttpServerAdvanced::WebUtility::JavaScriptStringEncode(std::string_view(), false);

        TEST_ASSERT_EQUAL_STRING("\"\\\"\\\\\\n\\t\\u0001\"", encodedWithQuotes.c_str());
        TEST_ASSERT_EQUAL_STRING("plain", encodedWithoutQuotes.c_str());
        TEST_ASSERT_EQUAL_STRING("\"\"", encodedEmptyWithQuotes.c_str());
        TEST_ASSERT_TRUE(encodedEmptyWithoutQuotes.empty());
    }

    void test_cors_string_view_overload_sets_headers()
    {
        std::string body("ok");
        auto response = HttpServerAdvanced::StringResponse::create(
            HttpServerAdvanced::HttpStatus::Ok(),
            std::move(body),
            std::initializer_list<HttpServerAdvanced::HttpHeader>{});
        auto filter = HttpServerAdvanced::CrossOriginRequestSharing(
            std::string_view("https://example.com"),
            std::string_view("GET,POST"),
            std::string_view("X-Test"),
            std::string_view("true"),
            std::string_view("X-Expose"),
            60,
            std::string_view("X-Requested-With"),
            std::string_view("OPTIONS"));

        response = filter(std::move(response));

        const auto allowOrigin = response->headers().find(std::string_view(HttpServerAdvanced::HttpHeaderNames::AccessControlAllowOrigin));
        TEST_ASSERT_TRUE(allowOrigin.has_value());
        TEST_ASSERT_EQUAL_STRING("https://example.com", std::string(allowOrigin->valueView()).c_str());

        const auto maxAge = response->headers().find(std::string_view(HttpServerAdvanced::HttpHeaderNames::AccessControlMaxAge));
        TEST_ASSERT_TRUE(maxAge.has_value());
        TEST_ASSERT_EQUAL_STRING("60", std::string(maxAge->valueView()).c_str());
    }

    void test_request_handler_factory_std_text_overloads_delegate_to_std_string()
    {
        HttpServerAdvanced::TestSupport::RecordingRequestHandlerFactory factory;

        factory.createResponse(HttpServerAdvanced::HttpStatus::Ok(), std::string_view("first"));
        TEST_ASSERT_EQUAL_STRING("first", factory.lastResponseBody().c_str());

        const std::string second("second");
        factory.createResponse(HttpServerAdvanced::HttpStatus::Ok(), second);
        TEST_ASSERT_EQUAL_STRING("second", factory.lastResponseBody().c_str());

        factory.createResponse(HttpServerAdvanced::HttpStatus::Ok(), "third");
        TEST_ASSERT_EQUAL_STRING("third", factory.lastResponseBody().c_str());
    }

    void test_http_context_items_exposes_std_string_keyed_map()
    {
        using ItemsType = std::remove_reference_t<decltype(std::declval<const HttpServerAdvanced::HttpContext &>().items())>;

        TEST_ASSERT_TRUE((std::is_same_v<ItemsType, std::map<std::string, std::any>>));
    }

    void test_handler_matcher_uses_standard_text_configuration()
    {
        HttpServerAdvanced::HandlerMatcher matcher(std::string_view("/files/*"), std::string_view("get,post"), {std::string_view("Application/Json"), std::string_view("text/plain")});

        TEST_ASSERT_EQUAL_STRING("/files/*", std::string(matcher.getUriPattern()).c_str());
        TEST_ASSERT_EQUAL_STRING("GET,POST", std::string(matcher.getAllowedMethods()).c_str());
        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(matcher.getAllowedContentTypes().size()));
        TEST_ASSERT_EQUAL_STRING("application/json", matcher.getAllowedContentTypes()[0].c_str());
        TEST_ASSERT_EQUAL_STRING("text/plain", matcher.getAllowedContentTypes()[1].c_str());

        matcher.setUriPattern(std::string_view("/assets/*"));
        matcher.setAllowedMethods(std::string_view("head"));
        matcher.setAllowedContentTypes({std::string_view("IMAGE/SVG+XML")});

        TEST_ASSERT_EQUAL_STRING("/assets/*", std::string(matcher.getUriPattern()).c_str());
        TEST_ASSERT_EQUAL_STRING("HEAD", std::string(matcher.getAllowedMethods()).c_str());
        TEST_ASSERT_EQUAL_STRING("image/svg+xml", matcher.getAllowedContentTypes()[0].c_str());
    }

    void test_handler_matcher_default_helpers_use_standard_text_inputs()
    {
        const std::string wildcardPattern = std::string("/files/") + HttpServerAdvanced::REQUEST_MATCHER_PATH_WILDCARD_CHAR;

        TEST_ASSERT_TRUE(HttpServerAdvanced::defaultCheckMethod(std::string_view("GET,POST"), std::string_view("GET")));
        TEST_ASSERT_FALSE(HttpServerAdvanced::defaultCheckMethod(std::string_view("GET,POST"), std::string_view("PUT")));

        TEST_ASSERT_TRUE(HttpServerAdvanced::defaultCheckUriPattern(std::string_view("/files/report.txt"), wildcardPattern));
        TEST_ASSERT_FALSE(HttpServerAdvanced::defaultCheckUriPattern(std::string_view("/files/report.txt"), std::string_view("/assets/*")));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_parse_query_parameters_uses_std_string_payloads);
        RUN_TEST(test_web_utility_std_string_view_overloads_remain_available);
        RUN_TEST(test_web_utility_base64_decode_to_std_string);
        RUN_TEST(test_web_utility_base64_decode_invalid_input_returns_empty_output);
        RUN_TEST(test_web_utility_uri_encoding_preserves_current_percent_encoding_behavior);
        RUN_TEST(test_web_utility_decode_uri_component_freezes_legacy_malformed_percent_handling);
        RUN_TEST(test_parse_query_parameters_freezes_empty_repeated_and_plus_decoded_values);
        RUN_TEST(test_uri_view_exposes_string_view_segments_and_query_payload);
        RUN_TEST(test_uri_view_handles_relative_query_only_fragment_only_and_authority_less_uris);
        RUN_TEST(test_header_collection_supports_string_view_lookups);
        RUN_TEST(test_header_collection_freezes_merge_overwrite_and_set_cookie_semantics);
        RUN_TEST(test_header_collection_preserves_insertion_slot_and_negative_lookup_behavior);
        RUN_TEST(test_http_header_preserves_standard_text_storage);
        RUN_TEST(test_http_header_supports_empty_copy_and_move_semantics);
        RUN_TEST(test_http_status_exposes_code_comparison_and_description_behavior);
        RUN_TEST(test_http_method_request_body_allowance_matches_current_policy);
        RUN_TEST(test_http_content_types_normalizes_extensions_and_falls_back_to_unknown);
        RUN_TEST(test_virtual_path_mapper_windows_normalizes_segments_and_names);
        RUN_TEST(test_virtual_path_mapper_windows_constructs_from_chroot_spec);
        RUN_TEST(test_virtual_path_mapper_posix_normalizes_segments_and_names);
        RUN_TEST(test_virtual_path_mapper_posix_constructs_from_chroot_spec);
        RUN_TEST(test_html_encode_returns_empty_for_null_or_empty_inputs);
        RUN_TEST(test_html_encode_preserves_expected_entities_without_progmem_helpers);
        RUN_TEST(test_html_attribute_encode_returns_empty_for_null_or_empty_inputs);
        RUN_TEST(test_html_attribute_encode_preserves_expected_entities_without_progmem_helpers);
        RUN_TEST(test_javascript_string_encode_escapes_control_characters_and_quotes);
        RUN_TEST(test_cors_string_view_overload_sets_headers);
        RUN_TEST(test_request_handler_factory_std_text_overloads_delegate_to_std_string);
        RUN_TEST(test_http_context_items_exposes_std_string_keyed_map);
        RUN_TEST(test_handler_matcher_uses_standard_text_configuration);
        RUN_TEST(test_handler_matcher_default_helpers_use_standard_text_inputs);
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