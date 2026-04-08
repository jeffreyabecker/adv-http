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

    inline OpenApiBuilder::OpenApiBuilder()
    {
        initializeDocument();
    }

    inline std::string_view OpenApiBuilder::getOpenApiDocument() const noexcept
    {
        refreshSerializedDocument();
        return serializedDocument_;
    }

    inline bool OpenApiBuilder::registerRoute(const std::string &route, JsonObject routeObject)
    {
        JsonObjectConst routeSource = routeObject;
        if (routeSource.isNull())
        {
            return false;
        }

        JsonObject paths = ensureObjectMember(document_.as<JsonObject>(), "paths");
        JsonObject target = ensureObjectMember(paths, route.c_str());
        if (!mergeObjects(target, routeSource))
        {
            return false;
        }

        serializedDocumentDirty_ = true;
        return true;
    }

    inline bool OpenApiBuilder::registerComponent(const std::string &name, JsonObject componentObject)
    {
        JsonObjectConst componentSource = componentObject;
        if (componentSource.isNull())
        {
            return false;
        }

        JsonObject components = ensureObjectMember(document_.as<JsonObject>(), "components");
        JsonObject target = ensureObjectMember(components, name.c_str());
        if (!mergeObjects(target, componentSource))
        {
            return false;
        }

        serializedDocumentDirty_ = true;
        return true;
    }

    inline bool OpenApiBuilder::mergeObjects(JsonObject target, JsonObjectConst source)
    {
        if (target.isNull() || source.isNull())
        {
            return false;
        }

        for (JsonPairConst item : source)
        {
            if (item.value().is<JsonObjectConst>())
            {
                JsonObject nestedTarget = ensureObjectMember(target, item.key().c_str());
                if (!mergeObjects(nestedTarget, item.value().as<JsonObjectConst>()))
                {
                    return false;
                }
                continue;
            }

            target[item.key()] = item.value();
        }

        return true;
    }

    inline JsonObject OpenApiBuilder::ensureObjectMember(JsonObject parent, const char *key)
    {
        return parent[key].is<JsonObject>() ? parent[key].as<JsonObject>() : parent[key].to<JsonObject>();
    }

    inline void OpenApiBuilder::initializeDocument()
    {
        document_.clear();

        const auto result = deserializeJson(document_, BaseOpenApiDocumentJson);
        if (result == DeserializationError::Ok)
        {
            serializedDocumentDirty_ = true;
            refreshSerializedDocument();
            return;
        }

        JsonObject root = document_.to<JsonObject>();
        root["openapi"] = "3.1.0";

        JsonObject info = root["info"].to<JsonObject>();
        info["title"] = "LumaLink HTTP API";
        info["version"] = "0.1.0";

        JsonObject paths = root["paths"].to<JsonObject>();
        JsonObject openApiRoute = paths["/api/openapi.json"].to<JsonObject>();
        JsonObject openApiGet = openApiRoute["get"].to<JsonObject>();
        openApiGet["operationId"] = "getOpenApiDocument";
        openApiGet["summary"] = "Get the emitted OpenAPI document.";

        root["components"].to<JsonObject>();

        serializedDocumentDirty_ = true;
        refreshSerializedDocument();
    }

    inline void OpenApiBuilder::refreshSerializedDocument() const
    {
        if (!serializedDocumentDirty_)
        {
            return;
        }

        serializedDocument_.clear();
        serializeJson(document_, serializedDocument_);
        serializedDocumentDirty_ = false;
    }

} // namespace lumalink::http::openapi

#include "RouteBuilder.h"
#endif