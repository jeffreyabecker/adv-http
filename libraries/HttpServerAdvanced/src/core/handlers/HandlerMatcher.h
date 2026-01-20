#pragma once
#include <Arduino.h>

#include "../HttpContext.h"
#include "../HttpRequest.h"
#include <initializer_list>
#include <functional>
namespace HttpServerAdvanced::Core
{
#ifndef HTTPSERVER_REQUEST_MATCHER_PATH_WILDCARD_CHAR
    constexpr char REQUEST_MATCHER_PATH_WILDCARD_CHAR = '?';
#else
    constexpr char REQUEST_MATCHER_PATH_WILDCARD_CHAR = HTTPSERVER_REQUEST_MATCHER_PATH_WILDCARD_CHAR;
#endif
    class HandlerMatcher
    {
    public:
        using MethodChecker = std::function<bool(const String &method, const String &allowedMethods)>;
        using UriPatternChecker = std::function<bool(const StringView &uri, const String &uriPattern)>;
        using ContentTypeChecker = std::function<bool(HttpRequest &request, const std::vector<String> &allowedContentTypes)>;
        using ParameterExtractor = std::function<std::vector<String>(HttpContext &context, const String &uriPattern)>;

    protected:
        String uriPattern_;
        String allowedMethods_;
        std::vector<String> allowedContentTypes_;

        MethodChecker methodChecker_;
        UriPatternChecker uriPatternChecker_;
        ContentTypeChecker contentTypeChecker_;
        ParameterExtractor parameterExtractor_;

        void fixStringCases();

    public:
        HandlerMatcher(const String &uriPattern, const String &allowedMethods = "", const std::initializer_list<String> &allowedContentTypes = {});

        HandlerMatcher(const char *uriPattern);

        // Constructor with custom functions
        HandlerMatcher(const String &uriPattern, MethodChecker methodChecker, UriPatternChecker uriPatternChecker,
            ContentTypeChecker contentTypeChecker, ParameterExtractor parameterExtractor,
            const String &allowedMethods = "", const std::initializer_list<String> &allowedContentTypes = {});

        // Setters for function objects
        void setMethodChecker(MethodChecker checker);
        void setUriPatternChecker(UriPatternChecker checker);
        void setContentTypeChecker(ContentTypeChecker checker);
        void setParameterExtractor(ParameterExtractor extractor);
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
        const ParameterExtractor &getParameterExtractor() const;

        ~HandlerMatcher() = default;

        bool operator()(HttpContext &context) const{
            return canHandle(context);
        }

        bool canHandle(HttpContext &context) const;
        std::vector<String> extractParameters(HttpContext &context) const;
    };
    class ParameterizedUri : public HandlerMatcher
    {
    public:
        ParameterizedUri(const String &uriPattern, const String &allowedMethods = "", const std::initializer_list<String> &allowedContentTypes = {});
    };
} // namespace HttpServerAdvanced::Core