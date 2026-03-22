#include "HandlerMatcher.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

#include "../core/HttpRequest.h"
#include "../util/UriView.h"

namespace HttpServerAdvanced
{
    namespace
    {
        std::string_view toStringView(const String &value)
        {
            return std::string_view(value.c_str(), value.length());
        }

        char toLowerAscii(char value)
        {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
        }

        bool startsWithIgnoreCase(std::string_view value, std::string_view prefix)
        {
            return value.size() >= prefix.size() &&
                   std::equal(prefix.begin(), prefix.end(), value.begin(), [](char lhs, char rhs)
                              { return toLowerAscii(lhs) == toLowerAscii(rhs); });
        }
    }

    // Default implementations
    bool defaultCheckMethod(std::string_view allowedMethods, std::string_view method)
    {
        if (allowedMethods.empty() || allowedMethods.find(method) != std::string_view::npos)
        {
            return true;
        }
        return false;
    }

    bool defaultCheckContentType(HttpRequest &context, const std::vector<String> &allowedContentTypes)
    {
        std::optional<HttpHeader> contentType = context.headers().find("Content-Type");
        if (!contentType.has_value())
        {
            return false;
        }

        bool matchFound = false;
        for (const auto &allowedCT : allowedContentTypes)
        {
            if (startsWithIgnoreCase(contentType->valueView(), toStringView(allowedCT)))
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

    bool defaultCheckUriPattern(std::string_view uri, std::string_view uriPattern)
    {
        auto v = UriView(uri);
        auto path = v.path();

        const char *t = path.data();
        const char *tEnd = t + path.size();
        const char *p = uriPattern.begin();
        const char *pEnd = p + uriPattern.length();
        const char *star = nullptr;
        const char *ts = nullptr;
        while (t != tEnd && p != pEnd)
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
        while (p != pEnd && *p == REQUEST_MATCHER_PATH_WILDCARD_CHAR)
            ++p;
        return p == pEnd;
    }

    RouteParameters defaultExtractParameters(HttpRequest &context, std::string_view uriPattern)
    {
        RouteParameters params;
        auto v = UriView(context.url());
        auto path = v.path();
        if (uriPattern.find('*') == std::string_view::npos)
        {
            return params;
        }

        const char *t = path.data();
        const char *tEnd = t + path.size();
        const char *p = uriPattern.data();
        const char *pEnd = p + uriPattern.size();
        std::string currentParam;

        while (t != tEnd && p != pEnd)
        {
            if (*p == REQUEST_MATCHER_PATH_WILDCARD_CHAR)
            {
                // Wildcard: capture until next REQUEST_MATCHER_PATH_DELIMITER or end of string
                currentParam.clear();
                ++p; // Move past the REQUEST_MATCHER_PATH_WILDCARD_CHAR

                // Capture characters until we hit the next REQUEST_MATCHER_PATH_DELIMITER in pattern or end of text
                while (t != tEnd && (*t != REQUEST_MATCHER_PATH_DELIMITER || (p != pEnd && *p != REQUEST_MATCHER_PATH_DELIMITER)))
                {
                    if (p != pEnd && *p == REQUEST_MATCHER_PATH_DELIMITER && *t == REQUEST_MATCHER_PATH_DELIMITER)
                    {
                        break;
                    }
                    currentParam.push_back(*t);
                    ++t;
                }

                if (!currentParam.empty())
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
        while (p != pEnd && *p == REQUEST_MATCHER_PATH_WILDCARD_CHAR)
        {
            currentParam.clear();
            ++p;
            while (t != tEnd && *t != REQUEST_MATCHER_PATH_DELIMITER)
            {
                currentParam.push_back(*t);
                ++t;
            }
            if (!currentParam.empty())
            {
                params.push_back(currentParam);
            }
            if (p != pEnd && t != tEnd && *p == REQUEST_MATCHER_PATH_DELIMITER && *t == REQUEST_MATCHER_PATH_DELIMITER)
            {
                ++t;
                ++p;
            }
        }

        return params;
    }

    // HandlerMatcher implementations
    HandlerMatcher::HandlerMatcher(const String &uriPattern, const String &allowedMethods, const std::initializer_list<String> &allowedContentTypes)
        : uriPattern_(uriPattern), allowedMethods_(allowedMethods), allowedContentTypes_(allowedContentTypes),
          methodChecker_(defaultCheckMethod), uriPatternChecker_(defaultCheckUriPattern),
          contentTypeChecker_(defaultCheckContentType), argsExtractor_(defaultExtractParameters)
    {
        allowedMethods_.toUpperCase();
        for (auto &ct : allowedContentTypes_)
        {
            ct.toLowerCase();
        }
    }

    void HandlerMatcher::fixStringCases()
    {
        allowedMethods_.toUpperCase();
        for (auto &ct : allowedContentTypes_)
        {
            ct.toLowerCase();
        }
    }

    HandlerMatcher::HandlerMatcher(const char *uriPattern) : HandlerMatcher(String(uriPattern)) {}

    HandlerMatcher::HandlerMatcher(const String &uriPattern, MethodChecker methodChecker, UriPatternChecker uriPatternChecker,
                                          ContentTypeChecker contentTypeChecker, ArgsExtractor argsExtractor,
                                          const String &allowedMethods, const std::initializer_list<String> &allowedContentTypes)
        : uriPattern_(uriPattern), allowedMethods_(allowedMethods), allowedContentTypes_(allowedContentTypes),
          methodChecker_(methodChecker), uriPatternChecker_(uriPatternChecker),
          contentTypeChecker_(contentTypeChecker), argsExtractor_(argsExtractor)
    {
        allowedMethods_.toUpperCase();
        for (auto &ct : allowedContentTypes_)
        {
            ct.toLowerCase();
        }
    }

    // Setters
    void HandlerMatcher::setMethodChecker(MethodChecker checker)
    {
        methodChecker_ = checker;
    }

    void HandlerMatcher::setUriPatternChecker(UriPatternChecker checker)
    {
        uriPatternChecker_ = checker;
    }

    void HandlerMatcher::setContentTypeChecker(ContentTypeChecker checker)
    {
        contentTypeChecker_ = checker;
    }

    void HandlerMatcher::setArgsExtractor(ArgsExtractor extractor)
    {
        argsExtractor_ = extractor;
    }

    void HandlerMatcher::setUriPattern(const String &uriPattern)
    {
        uriPattern_ = uriPattern;
    }

    void HandlerMatcher::setUriPattern(const char *uriPattern)
    {
        uriPattern_ = uriPattern != nullptr ? uriPattern : "";
    }

    void HandlerMatcher::setUriPattern(std::string_view uriPattern)
    {
        uriPattern_ = String(uriPattern.data(), uriPattern.size());
    }

    void HandlerMatcher::setAllowedMethods(const String &methods)
    {
        allowedMethods_ = methods;
        allowedMethods_.toUpperCase();
    }

    void HandlerMatcher::setAllowedMethods(const char *methods)
    {
        allowedMethods_ = methods != nullptr ? methods : "";
        allowedMethods_.toUpperCase();
    }

    void HandlerMatcher::setAllowedMethods(std::string_view methods)
    {
        allowedMethods_ = String(methods.data(), methods.size());
        allowedMethods_.toUpperCase();
    }

    void HandlerMatcher::setAllowedContentTypes(const std::initializer_list<String> &contentTypes)
    {
        allowedContentTypes_.clear();
        for (auto &ct : contentTypes)
        {
            String loweredCT = ct;
            loweredCT.toLowerCase();
            allowedContentTypes_.emplace_back(loweredCT);
        }
    }

    void HandlerMatcher::setAllowedContentTypes(const std::initializer_list<const char *> &contentTypes)
    {
        allowedContentTypes_.clear();
        for (const char *contentType : contentTypes)
        {
            String loweredCT = contentType != nullptr ? contentType : "";
            loweredCT.toLowerCase();
            allowedContentTypes_.emplace_back(loweredCT);
        }
    }

    // Getters
    const String &HandlerMatcher::getUriPattern() const
    {
        return uriPattern_;
    }

    const String &HandlerMatcher::getAllowedMethods() const
    {
        return allowedMethods_;
    }

    const std::vector<String> &HandlerMatcher::getAllowedContentTypes() const
    {
        return allowedContentTypes_;
    }

    const HandlerMatcher::MethodChecker &HandlerMatcher::getMethodChecker() const
    {
        return methodChecker_;
    }

    const HandlerMatcher::UriPatternChecker &HandlerMatcher::getUriPatternChecker() const
    {
        return uriPatternChecker_;
    }

    const HandlerMatcher::ContentTypeChecker &HandlerMatcher::getContentTypeChecker() const
    {
        return contentTypeChecker_;
    }

    const HandlerMatcher::ArgsExtractor &HandlerMatcher::getArgsExtractor() const
    {
        return argsExtractor_;
    }

    // Public methods
    bool HandlerMatcher::canHandle(HttpRequest &context) const
    {
        if (!allowedMethods_.isEmpty() && !methodChecker_(context.methodView(), toStringView(allowedMethods_)))
        {
            return false;
        }

        if (!uriPatternChecker_(context.urlView(), toStringView(uriPattern_)))
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

    RouteParameters HandlerMatcher::extractParameters(HttpRequest &context) const
    {
        return argsExtractor_(context, toStringView(uriPattern_));
    }

    // ParameterizedUri implementation
    ParameterizedUri::ParameterizedUri(const String &uriPattern, const String &allowedMethods, const std::initializer_list<String> &allowedContentTypes)
        : HandlerMatcher(uriPattern, allowedMethods, allowedContentTypes)
    {
    }

} // namespace HttpServerAdvanced


