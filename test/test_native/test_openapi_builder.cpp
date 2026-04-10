#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/lumalink/http/server/OperationBuilder.h"

#include <unity.h>

using namespace lumalink::http::core;
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
using namespace lumalink::http::openapi;
#endif

namespace
{
    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
    void test_response_supports_non_json_content_types()
    {
        JsonDocument document;
        JsonObject operation = document.to<JsonObject>();

        OperationBuilder(operation)
            .response(HttpStatus::Ok(), "Plain text response", HttpContentTypes::TextPlain)
            .response(HttpStatus::Accepted(), "Form response", HttpContentTypes::FormUrlencoded);

        JsonObject okResponse = operation["responses"]["200"].as<JsonObject>();
        JsonObject acceptedResponse = operation["responses"]["202"].as<JsonObject>();

        TEST_ASSERT_EQUAL_STRING("Plain text response", okResponse["description"] | "");
        TEST_ASSERT_TRUE(okResponse["content"][HttpContentTypes::TextPlain].is<JsonObject>());
        TEST_ASSERT_FALSE(okResponse["content"][HttpContentTypes::Json].is<JsonObject>());

        TEST_ASSERT_EQUAL_STRING("Form response", acceptedResponse["description"] | "");
        TEST_ASSERT_TRUE(acceptedResponse["content"][HttpContentTypes::FormUrlencoded].is<JsonObject>());
    }

    void test_request_body_and_response_can_attach_non_json_schemas()
    {
        JsonDocument document;
        JsonObject operation = document.to<JsonObject>();

        JsonDocument schemaDocument;
        JsonObject schemaDefinition = schemaDocument.to<JsonObject>();
        schemaDefinition["type"] = "string";

        OperationBuilder(operation)
            .requestBody(HttpContentTypes::FormUrlencoded, schemaDefinition, true)
            .response(HttpStatus::Ok(), "Plain text response", HttpContentTypes::TextPlain, schemaDefinition);

        JsonObject requestSchema = operation["requestBody"]["content"][HttpContentTypes::FormUrlencoded]["schema"].as<JsonObject>();
        JsonObject responseSchema = operation["responses"]["200"]["content"][HttpContentTypes::TextPlain]["schema"].as<JsonObject>();

        TEST_ASSERT_TRUE(operation["requestBody"]["required"].as<bool>());
        TEST_ASSERT_EQUAL_STRING("string", requestSchema["type"] | "");
        TEST_ASSERT_EQUAL_STRING("string", responseSchema["type"] | "");
    }

    void test_request_body_and_response_support_multiple_content_variants()
    {
        JsonDocument document;
        JsonObject operation = document.to<JsonObject>();

        JsonDocument formSchemaDocument;
        JsonObject formSchema = formSchemaDocument.to<JsonObject>();
        formSchema["type"] = "object";

        OperationBuilder(operation)
            .requestBody({OperationBuilder::ContentVariant::content(HttpContentTypes::TextPlain),
                          OperationBuilder::ContentVariant::inlineSchema(HttpContentTypes::FormUrlencoded, formSchema)},
                         true)
            .response(HttpStatus::Ok(),
                      "Multiple variants",
                      {OperationBuilder::ContentVariant::content(HttpContentTypes::TextPlain),
                       OperationBuilder::ContentVariant::schemaRef(HttpContentTypes::Json, "StatusPayload")});

        JsonObject requestContent = operation["requestBody"]["content"].as<JsonObject>();
        JsonObject responseContent = operation["responses"]["200"]["content"].as<JsonObject>();

        TEST_ASSERT_TRUE(requestContent[HttpContentTypes::TextPlain].is<JsonObject>());
        TEST_ASSERT_TRUE(requestContent[HttpContentTypes::FormUrlencoded]["schema"].is<JsonObject>());
        TEST_ASSERT_EQUAL_STRING("object", requestContent[HttpContentTypes::FormUrlencoded]["schema"]["type"] | "");

        TEST_ASSERT_TRUE(responseContent[HttpContentTypes::TextPlain].is<JsonObject>());
        TEST_ASSERT_EQUAL_STRING("#/components/schemas/StatusPayload",
                                 responseContent[HttpContentTypes::Json]["schema"]["$ref"] | "");
    }
#endif

    int runUnitySuite()
    {
        UNITY_BEGIN();
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
        RUN_TEST(test_response_supports_non_json_content_types);
        RUN_TEST(test_request_body_and_response_can_attach_non_json_schemas);
        RUN_TEST(test_request_body_and_response_support_multiple_content_variants);
#endif
        return UNITY_END();
    }
}

int run_test_openapi_builder()
{
    return lumalink::http::TestSupport::RunConsolidatedSuite(
        "openapi_builder",
        runUnitySuite,
        localSetUp,
        localTearDown);
}