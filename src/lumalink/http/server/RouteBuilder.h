#pragma once
#include "../core/Defines.h"
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1

#include <ArduinoJson.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "OpenApiBuilder.h"
#include "OperationBuilder.h"

namespace lumalink::http::openapi
{
    template <typename TOperationBuilder = OperationBuilder>
    class RouteBuilderT
    {
    public:
        RouteBuilderT(std::string route)
            : route_(std::move(route)), root_(document_.to<JsonObject>())
        {
        }

        TOperationBuilder operation(const std::string_view method)
        {
            TOperationBuilder builder(ensureOperation(method));
            for (const SharedParameter &parameter : sharedParameters_)
            {
                builder.parameter(parameter.name, parameter.location, parameter.schemaType, parameter.required,
                                  parameter.description);
            }

            return builder;
        }

        RouteBuilderT &parameter(const std::string_view name, const std::string_view location,
                                const std::string_view schemaType, const bool required = true,
                                const std::string_view description = {})
        {
            SharedParameter &parameter = upsertSharedParameter(name, location);
            parameter.schemaType = std::string(schemaType);
            parameter.required = required;
            parameter.description = std::string(description);

            applySharedParameter(parameter);
            return *this;
        }

        RouteBuilderT &requiredStringPathParameter(const std::string_view name, const std::string_view description = {})
        {
            return parameter(name, "path", "string", true, description);
        }

        RouteBuilderT &requiredStringQueryParameter(const std::string_view name, const std::string_view description = {})
        {
            return parameter(name, "query", "string", true, description);
        }

        TOperationBuilder get() { return operation("get"); }

        TOperationBuilder post() { return operation("post"); }

        TOperationBuilder put() { return operation("put"); }

        TOperationBuilder del() { return operation("delete"); }

        [[nodiscard]] JsonDocument &document() { return document_; }

        [[nodiscard]] JsonObject root() const { return root_; }


    private:
        struct SharedParameter
        {
            std::string name;
            std::string location;
            std::string schemaType;
            bool required = true;
            std::string description;
        };

        [[nodiscard]] static JsonObject ensureObjectMember(JsonObject parent, const char *key)
        {
            return parent[key].is<JsonObject>() ? parent[key].as<JsonObject>() : parent[key].to<JsonObject>();
        }

        [[nodiscard]] JsonObject ensureOperation(const std::string_view method)
        {
            std::string methodName(method);
            std::transform(methodName.begin(), methodName.end(), methodName.begin(), [](unsigned char ch)
                           { return static_cast<char>(std::tolower(ch)); });
            return ensureObjectMember(root_, methodName.c_str());
        }

        SharedParameter &upsertSharedParameter(const std::string_view name, const std::string_view location)
        {
            for (SharedParameter &parameter : sharedParameters_)
            {
                if (parameter.name == name && parameter.location == location)
                {
                    return parameter;
                }
            }

            sharedParameters_.push_back(SharedParameter{
                .name = std::string(name),
                .location = std::string(location),
            });
            return sharedParameters_.back();
        }

        void applySharedParameter(const SharedParameter &parameter)
        {
            for (JsonPair item : root_)
            {
                if (!item.value().is<JsonObject>())
                {
                    continue;
                }

                OperationBuilder(item.value().as<JsonObject>())
                    .parameter(parameter.name, parameter.location, parameter.schemaType, parameter.required,
                               parameter.description);
            }
        }

        std::string route_;
        JsonDocument document_;
        JsonObject root_;
        std::vector<SharedParameter> sharedParameters_;
    };

    using RouteBuilder = RouteBuilderT<>;

} // namespace lumalink::http::openapi
#endif