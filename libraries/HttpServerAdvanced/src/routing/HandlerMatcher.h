#pragma once
#include <Arduino.h>
#include "../core/Defines.h"
#include "../util/StringUtility.h"
#include "../util/UriView.h"
#include <initializer_list>
#include <functional>
namespace HttpServerAdvanced
{
    // Forward declaration
    class HttpRequest;
    constexpr char REQUEST_MATCHER_PATH_DELIMITER = '/';
    class HandlerMatcher
    {
    public:
        using MethodChecker = std::function<bool(const String &method, const String &allowedMethods)>;
        using UriPatternChecker = std::function<bool(const String &uri, const String &uriPattern)>;
        using ContentTypeChecker = std::function<bool(HttpRequest &context, const std::vector<String> &allowedContentTypes)>;
        using ArgsExtractor = std::function<std::vector<String>(HttpRequest &context, const String &uriPattern)>;

    protected:
        String uriPattern_;
        String allowedMethods_;
        std::vector<String> allowedContentTypes_;

        MethodChecker methodChecker_;
        UriPatternChecker uriPatternChecker_;
        ContentTypeChecker contentTypeChecker_;
        ArgsExtractor argsExtractor_;

        void fixStringCases();

    public:
        HandlerMatcher(const String &uriPattern, const String &allowedMethods = "", const std::initializer_list<String> &allowedContentTypes = {});

        HandlerMatcher(const char *uriPattern);

        // Constructor with custom functions
        HandlerMatcher(const String &uriPattern, MethodChecker methodChecker, UriPatternChecker uriPatternChecker,
                       ContentTypeChecker contentTypeChecker, ArgsExtractor argsExtractor,
                       const String &allowedMethods = "", const std::initializer_list<String> &allowedContentTypes = {});

        // Setters for function objects
        void setMethodChecker(MethodChecker checker);
        void setUriPatternChecker(UriPatternChecker checker);
        void setContentTypeChecker(ContentTypeChecker checker);
        void setArgsExtractor(ArgsExtractor extractor);
        void setUriPattern(const String &uriPattern);
        void setAllowedMethods(const String &methods);
        void setAllowedContentTypes(const std::initializer_list<String> &contentTypes);

        // Getters
        const String &getUriPattern() const;
        const String &getAllowedMethods() const;
        const std::vector<String> &getAllowedContentTypes() const;
        const MethodChecker &getMethodChecker() const;
        const UriPatternChecker &getUriPatternChecker() const;
        const ContentTypeChecker &getContentTypeChecker() const;
        const ArgsExtractor &getArgsExtractor() const;

        ~HandlerMatcher() = default;

        bool operator()(HttpRequest &context) const
        {
            return canHandle(context);
        }

        bool canHandle(HttpRequest &context) const;
        std::vector<String> extractParameters(HttpRequest &context) const;
    };
    class ParameterizedUri : public HandlerMatcher
    {
    public:
        ParameterizedUri(const String &uriPattern, const String &allowedMethods = "", const std::initializer_list<String> &allowedContentTypes = {});
    };

    // ========== Implementations ==========

    // Default implementations
    bool defaultCheckMethod(const String &allowedMethods, const String &method);

    bool defaultCheckContentType(HttpRequest &context, const std::vector<String> &allowedContentTypes);

    bool defaultCheckUriPattern(const String &uri, const String &uriPattern);

    std::vector<String> defaultExtractParameters(HttpRequest &context, const String &uriPattern);

} // namespace HttpServerAdvanced

// Include HttpRequest after class definition to resolve forward declaration
#include "../core/HttpRequest.h"



