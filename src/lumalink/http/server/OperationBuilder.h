#pragma once
#include "../core/Defines.h"
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1

#include <ArduinoJson.h>

#include <initializer_list>
#include <string>
#include <string_view>

#include "../core/HttpContentTypes.h"
#include "../core/HttpStatus.h"

namespace lumalink::http::openapi
{
    namespace ParameterLocation
    {
        inline constexpr const char *Path = "path";
        inline constexpr const char *Query = "query";
        inline constexpr const char *Header = "header";
        inline constexpr const char *Cookie = "cookie";
    }

    class OperationBuilder
    {
    public:
        struct ContentVariant
        {
            enum class Kind
            {
                ContentOnly,
                SchemaRef,
                InlineSchema
            };

            std::string contentType;
            Kind kind = Kind::ContentOnly;
            std::string schemaName;
            JsonObjectConst schemaDefinition;

            ContentVariant() = default;

            ContentVariant(std::string contentTypeValue,
                           const Kind kindValue,
                           std::string schemaNameValue = {},
                           JsonObjectConst schemaDefinitionValue = JsonObjectConst())
                : contentType(std::move(contentTypeValue)),
                  kind(kindValue),
                  schemaName(std::move(schemaNameValue)),
                  schemaDefinition(schemaDefinitionValue)
            {
            }

            [[nodiscard]] static ContentVariant content(const std::string_view contentType)
            {
                return ContentVariant(std::string(contentType), Kind::ContentOnly);
            }

            [[nodiscard]] static ContentVariant schemaRef(const std::string_view contentType,
                                                          const std::string_view schemaName)
            {
                return ContentVariant(std::string(contentType), Kind::SchemaRef, std::string(schemaName));
            }

            [[nodiscard]] static ContentVariant inlineSchema(const std::string_view contentType,
                                                             JsonObject schemaDefinition)
            {
                return ContentVariant(std::string(contentType), Kind::InlineSchema, {}, schemaDefinition);
            }
        };

        explicit OperationBuilder(JsonObject operation) : operation_(operation) {}

        OperationBuilder &operationId(const std::string_view value)
        {
            operation_["operationId"] = std::string(value);
            return *this;
        }

        OperationBuilder &summary(const std::string_view value)
        {
            operation_["summary"] = std::string(value);
            return *this;
        }

        OperationBuilder &description(const std::string_view value)
        {
            operation_["description"] = std::string(value);
            return *this;
        }

        OperationBuilder &requestBody(const std::string_view contentType, const bool required = true)
        {
            JsonObject requestBody = ensureObjectMember(operation_, "requestBody");
            requestBody["required"] = required;

            JsonObject mediaType = ensureMediaTypeObject(requestBody, contentType);
            (void)mediaType;
            return *this;
        }

        OperationBuilder &requestBody(const std::initializer_list<ContentVariant> variants, const bool required = true)
        {
            for (const ContentVariant &variant : variants)
            {
                applyRequestBodyVariant(variant, required);
            }

            return *this;
        }

        OperationBuilder &requestBody(const std::string_view contentType,
                                      const std::string_view schemaName,
                                      const bool required = true)
        {
            JsonObject schema = ensureRequestBodySchema(contentType, required);
            schema["$ref"] = schemaRef(schemaName);
            return *this;
        }

        OperationBuilder &requestBody(const std::string_view contentType,
                                      JsonObject schemaDefinition,
                                      const bool required = true)
        {
            return requestBody(contentType, JsonObjectConst(schemaDefinition), required);
        }

        OperationBuilder &requestBody(const std::string_view contentType,
                                      JsonObjectConst schemaDefinition,
                                      const bool required = true)
        {
            JsonObject schema = ensureRequestBodySchema(contentType, required);
            schema.clear();
            copyObject(schema, schemaDefinition);
            return *this;
        }

        OperationBuilder &jsonRequestBody(const std::string_view schemaName, const bool required = true)
        {
            return requestBody(core::HttpContentTypes::Json, schemaName, required);
        }

        OperationBuilder &jsonRequestBody(JsonObject schemaDefinition, const bool required = true)
        {
            return requestBody(core::HttpContentTypes::Json, schemaDefinition, required);
        }

        OperationBuilder &response(const lumalink::http::core::HttpStatus &statusCode,
                                   const std::string_view description,
                                   const std::string_view contentType)
        {
            JsonObject response = ensureResponse(statusCode, description);
            JsonObject mediaType = ensureMediaTypeObject(response, contentType);
            (void)mediaType;
            return *this;
        }

        OperationBuilder &response(const lumalink::http::core::HttpStatus &statusCode,
                                   const std::string_view description,
                                   const std::initializer_list<ContentVariant> variants)
        {
            for (const ContentVariant &variant : variants)
            {
                applyResponseVariant(statusCode, description, variant);
            }

            return *this;
        }

        OperationBuilder &response(const lumalink::http::core::HttpStatus &statusCode,
                                   const std::string_view description,
                                   const std::string_view contentType,
                                   const std::string_view schemaName)
        {
            JsonObject schema = ensureResponseSchema(statusCode, description, contentType);
            schema["$ref"] = schemaRef(schemaName);
            return *this;
        }

        OperationBuilder &response(const lumalink::http::core::HttpStatus &statusCode,
                                   const std::string_view description,
                                   const std::string_view contentType,
                                   JsonObject schemaDefinition)
        {
            return response(statusCode, description, contentType, JsonObjectConst(schemaDefinition));
        }

        OperationBuilder &response(const lumalink::http::core::HttpStatus &statusCode,
                                   const std::string_view description,
                                   const std::string_view contentType,
                                   JsonObjectConst schemaDefinition)
        {
            JsonObject schema = ensureResponseSchema(statusCode, description, contentType);
            schema.clear();
            copyObject(schema, schemaDefinition);
            return *this;
        }

        OperationBuilder &jsonResponse(const lumalink::http::core::HttpStatus &statusCode,
                                       const std::string_view description,
                                       const std::string_view schemaName)
        {
            return response(statusCode, description, core::HttpContentTypes::Json, schemaName);
        }

        OperationBuilder &jsonResponse(const lumalink::http::core::HttpStatus &statusCode,
                                       const std::string_view description,
                                       JsonObject schemaDefinition)
        {
            return response(statusCode, description, core::HttpContentTypes::Json, schemaDefinition);
        }

        OperationBuilder &parameter(const std::string_view name, const std::string_view location,
                                    const std::string_view schemaType, const bool required = true,
                                    const std::string_view description = {})
        {
            JsonObject parameter = ensureParameter(operation_, name, location);
            parameter["name"] = std::string(name);
            parameter["in"] = std::string(location);
            parameter["required"] = required;
            if (!description.empty())
            {
                parameter["description"] = std::string(description);
            }

            JsonObject schema = ensureObjectMember(parameter, "schema");
            schema["type"] = std::string(schemaType);
            return *this;
        }

        [[nodiscard]] JsonObject json() const { return operation_; }

    private:
        [[nodiscard]] static JsonObject ensureObjectMember(JsonObject parent, const char *key)
        {
            return parent[key].is<JsonObject>() ? parent[key].as<JsonObject>() : parent[key].to<JsonObject>();
        }

        [[nodiscard]] static JsonObject ensureObjectMember(JsonObject parent, const std::string_view key)
        {
            const std::string memberKey(key);
            return parent[memberKey.c_str()].is<JsonObject>() ? parent[memberKey.c_str()].as<JsonObject>()
                                                              : parent[memberKey.c_str()].to<JsonObject>();
        }

        [[nodiscard]] static JsonArray ensureArrayMember(JsonObject parent, const char *key)
        {
            return parent[key].is<JsonArray>() ? parent[key].as<JsonArray>() : parent[key].to<JsonArray>();
        }

        [[nodiscard]] static JsonObject ensureMediaTypeObject(JsonObject parent, const std::string_view contentType)
        {
            JsonObject content = ensureObjectMember(parent, "content");
            return ensureObjectMember(content, contentType);
        }

        [[nodiscard]] JsonObject ensureRequestBodySchema(const std::string_view contentType, const bool required)
        {
            JsonObject requestBody = ensureObjectMember(operation_, "requestBody");
            requestBody["required"] = required;
            JsonObject mediaType = ensureMediaTypeObject(requestBody, contentType);
            return ensureObjectMember(mediaType, "schema");
        }

        void applyRequestBodyVariant(const ContentVariant &variant, const bool required)
        {
            switch (variant.kind)
            {
                case ContentVariant::Kind::ContentOnly:
                    requestBody(variant.contentType, required);
                    break;
                case ContentVariant::Kind::SchemaRef:
                    requestBody(variant.contentType, variant.schemaName, required);
                    break;
                case ContentVariant::Kind::InlineSchema:
                    requestBody(variant.contentType, variant.schemaDefinition, required);
                    break;
            }
        }

        [[nodiscard]] JsonObject ensureResponse(const lumalink::http::core::HttpStatus &statusCode,
                                                const std::string_view description)
        {
            JsonObject responses = ensureObjectMember(operation_, "responses");
            const std::string statusCodeKey = std::to_string(statusCode.code());
            JsonObject response = ensureObjectMember(responses, statusCodeKey);
            response["description"] = std::string(description);
            return response;
        }

        [[nodiscard]] JsonObject ensureResponseSchema(const lumalink::http::core::HttpStatus &statusCode,
                                                      const std::string_view description,
                                                      const std::string_view contentType)
        {
            JsonObject response = ensureResponse(statusCode, description);
            JsonObject mediaType = ensureMediaTypeObject(response, contentType);
            return ensureObjectMember(mediaType, "schema");
        }

        void applyResponseVariant(const lumalink::http::core::HttpStatus &statusCode,
                                  const std::string_view description,
                                  const ContentVariant &variant)
        {
            switch (variant.kind)
            {
                case ContentVariant::Kind::ContentOnly:
                    response(statusCode, description, variant.contentType);
                    break;
                case ContentVariant::Kind::SchemaRef:
                    response(statusCode, description, variant.contentType, variant.schemaName);
                    break;
                case ContentVariant::Kind::InlineSchema:
                    response(statusCode, description, variant.contentType, variant.schemaDefinition);
                    break;
            }
        }

        static void copyObject(JsonObject target, JsonObjectConst source)
        {
            for (JsonPairConst item : source)
            {
                if (item.value().is<JsonObjectConst>())
                {
                    copyObject(ensureObjectMember(target, item.key().c_str()), item.value().as<JsonObjectConst>());
                    continue;
                }

                target[item.key()] = item.value();
            }
        }

        [[nodiscard]] static JsonObject ensureParameter(JsonObject operation, const std::string_view name,
                                                        const std::string_view location)
        {
            JsonArray parameters = ensureArrayMember(operation, "parameters");
            for (JsonObject parameter : parameters)
            {
                const char *existingName = parameter["name"] | "";
                const char *existingLocation = parameter["in"] | "";
                if (name == existingName && location == existingLocation)
                {
                    return parameter;
                }
            }

            return parameters.add<JsonObject>();
        }

        [[nodiscard]] static std::string schemaRef(const std::string_view schemaName)
        {
            return "#/components/schemas/" + std::string(schemaName);
        }

        JsonObject operation_;
    };

} // namespace lumalink::http::openapi
#endif