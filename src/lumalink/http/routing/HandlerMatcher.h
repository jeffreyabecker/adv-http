#pragma once
#include "../core/Defines.h"
#include "../handlers/HandlerRestrictions.h"

#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace lumalink::http::routing
{
    using lumalink::http::core::HttpRequestContext;
    using lumalink::http::handlers::RouteParameters;

    constexpr char REQUEST_MATCHER_PATH_DELIMITER = '/';
    constexpr char REQUEST_MATCHER_PARAMETER_PREFIX = ':';
    constexpr char REQUEST_MATCHER_GLOB_WILDCARD_CHAR = '*';
    class HandlerMatcher
    {
    public:
        using MethodChecker = std::function<bool(std::string_view allowedMethods, std::string_view method)>;
        using UriPatternChecker = std::function<bool(std::string_view uri, std::string_view uriPattern)>;
        using ContentTypeChecker = std::function<bool(HttpRequestContext &context, const std::vector<std::string> &allowedContentTypes)>;
        using ArgsExtractor = std::function<RouteParameters(HttpRequestContext &context, std::string_view uriPattern)>;

    protected:
        std::string uriPattern_;
        std::string allowedMethods_;
        std::vector<std::string> allowedContentTypes_;

        MethodChecker methodChecker_;
        UriPatternChecker uriPatternChecker_;
        ContentTypeChecker contentTypeChecker_;
        ArgsExtractor argsExtractor_;

        void fixStringCases();

    public:
        HandlerMatcher(std::string_view uriPattern, std::string_view allowedMethods = "", const std::initializer_list<std::string_view> &allowedContentTypes = {});

        HandlerMatcher(const char *uriPattern);

        // Constructor with custom functions
        HandlerMatcher(std::string_view uriPattern, MethodChecker methodChecker, UriPatternChecker uriPatternChecker,
                       ContentTypeChecker contentTypeChecker, ArgsExtractor argsExtractor,
                       std::string_view allowedMethods = "", const std::initializer_list<std::string_view> &allowedContentTypes = {});

        // Setters for function objects
        void setMethodChecker(MethodChecker checker);
        void setUriPatternChecker(UriPatternChecker checker);
        void setContentTypeChecker(ContentTypeChecker checker);
        void setArgsExtractor(ArgsExtractor extractor);
        void setUriPattern(const char *uriPattern);
        void setUriPattern(std::string_view uriPattern);
        void setAllowedMethods(const char *methods);
        void setAllowedMethods(std::string_view methods);
        void setAllowedContentTypes(const std::initializer_list<const char *> &contentTypes);
        void setAllowedContentTypes(const std::initializer_list<std::string_view> &contentTypes);

        // Getters
        std::string_view getUriPattern() const;
        std::string_view getAllowedMethods() const;
        const std::vector<std::string> &getAllowedContentTypes() const;
        const MethodChecker &getMethodChecker() const;
        const UriPatternChecker &getUriPatternChecker() const;
        const ContentTypeChecker &getContentTypeChecker() const;
        const ArgsExtractor &getArgsExtractor() const;

        ~HandlerMatcher() = default;

        bool operator()(HttpRequestContext &context) const
        {
            return canHandle(context);
        }

        bool canHandle(HttpRequestContext &context) const;
        RouteParameters extractParameters(HttpRequestContext &context) const;
    };
    class ParameterizedUri : public HandlerMatcher
    {
    public:
        ParameterizedUri(std::string_view uriPattern, std::string_view allowedMethods = "", const std::initializer_list<std::string_view> &allowedContentTypes = {});
    };

    // ========== Implementations ==========

    // Default implementations
    bool defaultCheckMethod(std::string_view allowedMethods, std::string_view method);

    bool defaultCheckContentType(HttpRequestContext &context, const std::vector<std::string> &allowedContentTypes);

    bool defaultCheckUriPattern(std::string_view uri, std::string_view uriPattern);

    RouteParameters defaultExtractParameters(HttpRequestContext &context, std::string_view uriPattern);

} // namespace lumalink::http::routing



