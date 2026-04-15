#pragma once
#include "../core/Defines.h"
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1

#include <ArduinoJson.h>

#include <string>
#include <string_view>
#include "OperationBuilder.h"

namespace lumalink::http::openapi
{

    inline constexpr const char BaseOpenApiDocumentJson[] = R"json({
    "openapi": "3.1.0",
    "info": {
        "title": "LumaLink HTTP API",
        "version": "0.1.0"
    },
    "paths": {
        "/api/openapi.json": {
            "get": {
                "operationId": "getOpenApiDocument",
                "summary": "Get the emitted OpenAPI document."
            }
        }
    },
    "components": {}
})json";

    class OpenApiBuilder
    {
    public:
        OpenApiBuilder();
        ~OpenApiBuilder() = default;

        // Returns a view over the emitted OpenAPI document JSON text.
        // Implementations must ensure the returned view remains valid for the caller's use.
        [[nodiscard]] std::string_view getOpenApiDocument() const noexcept;

        // Register a full route description encoded as an ArduinoJson `JsonObject`.
        // Implementations should merge the provided object under a route-unique key.
        bool registerRoute(const std::string &route, JsonObject routeObject);

        // Register a named component (schema, parameter, etc.) encoded as a `JsonObject`.
        bool registerComponent(const std::string &name, JsonObject componentObject);

        bool registerSchemaComponents(JsonObject schemaComponents) { return registerComponent("schemas", schemaComponents); }

    private:
        static bool mergeObjects(JsonObject target, JsonObjectConst source);
        static JsonObject ensureObjectMember(JsonObject parent, const char *key);

        void initializeDocument();
        void refreshSerializedDocument() const;

        JsonDocument document_;
        mutable std::string serializedDocument_;
        mutable bool serializedDocumentDirty_ = true;
    };

} // namespace lumalink::http::openapi

#include "RouteBuilder.h"
#endif