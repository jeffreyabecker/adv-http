#include "./HandlerMatcher.h"
#include "./Defines.h"
#include "./StringUtility.h"
#include "./HttpRequest.h"
#include "./UriView.h"
namespace HttpServerAdvanced
{
    constexpr char REQUEST_MATCHER_PATH_DELIMITER = '/';
    // Default implementations
    bool defaultCheckMethod(const String &allowedMethods, const String &method)
    {
        if (allowedMethods.isEmpty() || allowedMethods.indexOf(method) != -1)
        {
            return true;
        }
        return false;
    }

    bool defaultCheckContentType(HttpRequest &request, const std::vector<String> &allowedContentTypes)
    {
        std::optional<HttpHeader> contentType = request.headers().find("Content-Type");
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
    bool defaultCheckUriPattern(const String &uri, const String &uriPattern)
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

    std::vector<String> defaultExtractParameters(HttpContext &context, const String &uriPattern)
    {
        std::vector<String> params;
        auto v = UriView(context.request().url());
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

    // Constructors
    HandlerMatcher::HandlerMatcher(const String &uriPattern, const String &allowedMethods, const std::initializer_list<String> &allowedContentTypes)
        : uriPattern_(uriPattern), allowedMethods_(allowedMethods), allowedContentTypes_(allowedContentTypes),
          methodChecker_(defaultCheckMethod), uriPatternChecker_(defaultCheckUriPattern),
          contentTypeChecker_(defaultCheckContentType), parameterExtractor_(defaultExtractParameters)
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
             ContentTypeChecker contentTypeChecker, ParameterExtractor parameterExtractor,
             const String &allowedMethods, const std::initializer_list<String> &allowedContentTypes)
        : uriPattern_(uriPattern), allowedMethods_(allowedMethods), allowedContentTypes_(allowedContentTypes),
          methodChecker_(methodChecker), uriPatternChecker_(uriPatternChecker),
          contentTypeChecker_(contentTypeChecker), parameterExtractor_(parameterExtractor)
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

    void HandlerMatcher::setParameterExtractor(ParameterExtractor extractor)
    {
        parameterExtractor_ = extractor;
    }

    void HandlerMatcher::setUriPattern(const String &uriPattern)
    {
        uriPattern_ = uriPattern;
    }

    void HandlerMatcher::setAllowedMethods(const String &methods)
    {
        allowedMethods_ = methods;
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

    const HandlerMatcher::ParameterExtractor &HandlerMatcher::getParameterExtractor() const
    {
        return parameterExtractor_;
    }

    // Public methods
    bool HandlerMatcher::canHandle(HttpContext &context) const
    {
        auto &request = context.request();
        if (!allowedMethods_.isEmpty() && !methodChecker_(request.method(), allowedMethods_))
        {
            return false;
        }

        auto requestUri = request.url();
        if (!uriPatternChecker_(requestUri, uriPattern_))
        {
            return false;
        }

        if (!allowedContentTypes_.empty())
        {
            if (!contentTypeChecker_(request, allowedContentTypes_))
            {
                return false;
            }
        }
        return true;
    }

    std::vector<String> HandlerMatcher::extractParameters(HttpContext &context) const
    {
        return parameterExtractor_(context, uriPattern_);
    }

} // namespace HttpServerAdvanced