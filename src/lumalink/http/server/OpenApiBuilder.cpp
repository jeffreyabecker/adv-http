#include "OpenApiBuilder.h"

#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1

namespace lumalink::http::openapi
{
    OpenApiBuilder::OpenApiBuilder()
    {
        initializeDocument();
    }

    std::string_view OpenApiBuilder::getOpenApiDocument() const noexcept
    {
        refreshSerializedDocument();
        return serializedDocument_;
    }

    bool OpenApiBuilder::registerRoute(const std::string &route, JsonObject routeObject)
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

    bool OpenApiBuilder::registerComponent(const std::string &name, JsonObject componentObject)
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

    bool OpenApiBuilder::mergeObjects(JsonObject target, JsonObjectConst source)
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

    JsonObject OpenApiBuilder::ensureObjectMember(JsonObject parent, const char *key)
    {
        return parent[key].is<JsonObject>() ? parent[key].as<JsonObject>() : parent[key].to<JsonObject>();
    }

    void OpenApiBuilder::initializeDocument()
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

    void OpenApiBuilder::refreshSerializedDocument() const
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

#endif
