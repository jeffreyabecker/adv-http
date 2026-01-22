#pragma once
#include <Arduino.h>
#include "./Defines.h"
#include "./StringUtility.h"
#include "./UriView.h"
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
        ArgsExtractor ArgsExtractor_;

        void fixStringCases();

    public:
        HandlerMatcher(const String &uriPattern, const String &allowedMethods = "", const std::initializer_list<String> &allowedContentTypes = {});

        HandlerMatcher(const char *uriPattern);

        // Constructor with custom functions
        HandlerMatcher(const String &uriPattern, MethodChecker methodChecker, UriPatternChecker uriPatternChecker,
                       ContentTypeChecker contentTypeChecker, ArgsExtractor ArgsExtractor,
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
    inline bool defaultCheckMethod(const String &allowedMethods, const String &method)
    {
        if (allowedMethods.isEmpty() || allowedMethods.indexOf(method) != -1)
        {
            return true;
        }
        return false;
    }

    inline bool defaultCheckContentType(HttpRequest &context, const std::vector<String> &allowedContentTypes)
    {
        std::optional<HttpHeader> contentType = context.headers().find("Content-Type");
        if (!contentType.has_value())
        {
            return false;
        }

        bool matchFound = false;
        for (const auto &allowedCT : allowedContentTypes)
        {
            if (StringUtil::startsWith(contentType->value(), allowedCT, true))
            {
                matchFound = true;
                break;
            }
        }
        if (!matchFound)
        {
            return false;
        }
        return true;
    }

    inline bool defaultCheckUriPattern(const String &uri, const String &uriPattern)
    {
        auto v = UriView(uri);
        auto path = v.path();

        const char *t = path.begin();
        const char *p = uriPattern.begin();
        const char *star = nullptr;
        const char *ts = nullptr;
        while (t != path.end() && p != uriPattern.end())
        {
            if (*p == *t)
            {
                ++t;
                ++p;
            }
            else if (*p == REQUEST_MATCHER_PATH_WILDCARD_CHAR)
            {
                star = p;
                ts = t;
                ++p;
            }
            else if (star)
            {
                p = star + 1;
                t = ++ts;
            }
            else
            {
                return false;
            }
        }
        while (*p == REQUEST_MATCHER_PATH_WILDCARD_CHAR)
            ++p;
        return !*p;
    }

    inline std::vector<String> defaultExtractParameters(HttpRequest &context, const String &uriPattern)
    {
        std::vector<String> params;
        auto v = UriView(context.url());
        auto path = v.path();
        if (StringUtil::indexOf(uriPattern, "*") == -1)
        {
            return params;
        }

        const char *t = path.begin();
        const char *p = uriPattern.begin();
        String currentParam;

        while (t != path.end() && p != uriPattern.end())
        {
            if (*p == REQUEST_MATCHER_PATH_WILDCARD_CHAR)
            {
                // Wildcard: capture until next REQUEST_MATCHER_PATH_DELIMITER or end of string
                currentParam = String();
                ++p; // Move past the REQUEST_MATCHER_PATH_WILDCARD_CHAR

                // Capture characters until we hit the next REQUEST_MATCHER_PATH_DELIMITER in pattern or end of text
                while (t != path.end() && (*t != REQUEST_MATCHER_PATH_DELIMITER || (p != uriPattern.end() && *p != REQUEST_MATCHER_PATH_DELIMITER)))
                {
                    if (*p == REQUEST_MATCHER_PATH_DELIMITER && *t == REQUEST_MATCHER_PATH_DELIMITER)
                    {
                        break;
                    }
                    currentParam += *t;
                    ++t;
                }

                if (!currentParam.isEmpty())
                {
                    params.push_back(currentParam);
                }
            }
            else if (*p == REQUEST_MATCHER_PATH_DELIMITER && *t == REQUEST_MATCHER_PATH_DELIMITER)
            {
                // Both have REQUEST_MATCHER_PATH_DELIMITER, move forward
                ++t;
                ++p;
            }
            else if (*p == *t)
            {
                // Characters match
                ++t;
                ++p;
            }
            else
            {
                // Mismatch
                break;
            }
        }

        // Handle trailing wildcards
        while (*p == REQUEST_MATCHER_PATH_WILDCARD_CHAR)
        {
            currentParam = String();
            ++p;
            while (*t && *t != REQUEST_MATCHER_PATH_DELIMITER)
            {
                currentParam += *t;
                ++t;
            }
            if (!currentParam.isEmpty())
            {
                params.push_back(currentParam);
            }
            if (*p == REQUEST_MATCHER_PATH_DELIMITER && *t == REQUEST_MATCHER_PATH_DELIMITER)
            {
                ++t;
                ++p;
            }
        }

        return params;
    }

    // HandlerMatcher implementations
    inline HandlerMatcher::HandlerMatcher(const String &uriPattern, const String &allowedMethods, const std::initializer_list<String> &allowedContentTypes)
        : uriPattern_(uriPattern), allowedMethods_(allowedMethods), allowedContentTypes_(allowedContentTypes),
          methodChecker_(defaultCheckMethod), uriPatternChecker_(defaultCheckUriPattern),
          contentTypeChecker_(defaultCheckContentType), ArgsExtractor_(defaultExtractParameters)
    {
        allowedMethods_.toUpperCase();
        for (auto &ct : allowedContentTypes_)
        {
            ct.toLowerCase();
        }
    }

    inline void HandlerMatcher::fixStringCases()
    {
        allowedMethods_.toUpperCase();
        for (auto &ct : allowedContentTypes_)
        {
            ct.toLowerCase();
        }
    }

    inline HandlerMatcher::HandlerMatcher(const char *uriPattern) : HandlerMatcher(String(uriPattern)) {}

    inline HandlerMatcher::HandlerMatcher(const String &uriPattern, MethodChecker methodChecker, UriPatternChecker uriPatternChecker,
                                          ContentTypeChecker contentTypeChecker, ArgsExtractor ArgsExtractor,
                                          const String &allowedMethods, const std::initializer_list<String> &allowedContentTypes)
        : uriPattern_(uriPattern), allowedMethods_(allowedMethods), allowedContentTypes_(allowedContentTypes),
          methodChecker_(methodChecker), uriPatternChecker_(uriPatternChecker),
          contentTypeChecker_(contentTypeChecker), ArgsExtractor_(ArgsExtractor)
    {
        allowedMethods_.toUpperCase();
        for (auto &ct : allowedContentTypes_)
        {
            ct.toLowerCase();
        }
    }

    // Setters
    inline void HandlerMatcher::setMethodChecker(MethodChecker checker)
    {
        methodChecker_ = checker;
    }

    inline void HandlerMatcher::setUriPatternChecker(UriPatternChecker checker)
    {
        uriPatternChecker_ = checker;
    }

    inline void HandlerMatcher::setContentTypeChecker(ContentTypeChecker checker)
    {
        contentTypeChecker_ = checker;
    }

    inline void HandlerMatcher::setArgsExtractor(ArgsExtractor extractor)
    {
        ArgsExtractor_ = extractor;
    }

    inline void HandlerMatcher::setUriPattern(const String &uriPattern)
    {
        uriPattern_ = uriPattern;
    }

    inline void HandlerMatcher::setAllowedMethods(const String &methods)
    {
        allowedMethods_ = methods;
        allowedMethods_.toUpperCase();
    }

    inline void HandlerMatcher::setAllowedContentTypes(const std::initializer_list<String> &contentTypes)
    {
        allowedContentTypes_.clear();
        for (auto &ct : contentTypes)
        {
            String loweredCT = ct;
            loweredCT.toLowerCase();
            allowedContentTypes_.emplace_back(loweredCT);
        }
    }

    // Getters
    inline const String &HandlerMatcher::getUriPattern() const
    {
        return uriPattern_;
    }

    inline const String &HandlerMatcher::getAllowedMethods() const
    {
        return allowedMethods_;
    }

    inline const std::vector<String> &HandlerMatcher::getAllowedContentTypes() const
    {
        return allowedContentTypes_;
    }

    inline const HandlerMatcher::MethodChecker &HandlerMatcher::getMethodChecker() const
    {
        return methodChecker_;
    }

    inline const HandlerMatcher::UriPatternChecker &HandlerMatcher::getUriPatternChecker() const
    {
        return uriPatternChecker_;
    }

    inline const HandlerMatcher::ContentTypeChecker &HandlerMatcher::getContentTypeChecker() const
    {
        return contentTypeChecker_;
    }

    inline const HandlerMatcher::ArgsExtractor &HandlerMatcher::getArgsExtractor() const
    {
        return ArgsExtractor_;
    }

    // Public methods
    inline bool HandlerMatcher::canHandle(HttpRequest &context) const
    {
        if (!allowedMethods_.isEmpty() && !methodChecker_(context.method(), allowedMethods_))
        {
            return false;
        }

        auto requestUri = context.url();
        if (!uriPatternChecker_(requestUri, uriPattern_))
        {
            return false;
        }

        if (!allowedContentTypes_.empty())
        {
            if (!contentTypeChecker_(context, allowedContentTypes_))
            {
                return false;
            }
        }
        return true;
    }

    inline std::vector<String> HandlerMatcher::extractParameters(HttpRequest &context) const
    {
        return ArgsExtractor_(context, uriPattern_);
    }

    // ParameterizedUri implementation
    inline ParameterizedUri::ParameterizedUri(const String &uriPattern, const String &allowedMethods, const std::initializer_list<String> &allowedContentTypes)
        : HandlerMatcher(uriPattern, allowedMethods, allowedContentTypes)
    {
    }

} // namespace HttpServerAdvanced

// Include HttpRequest after class definition to resolve forward declaration
#include "./HttpRequest.h"
