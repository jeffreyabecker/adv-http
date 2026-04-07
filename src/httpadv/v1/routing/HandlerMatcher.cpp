#include "HandlerMatcher.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include "../core/HttpRequestContext.h"
#include "../util/UriView.h"

namespace httpadv::v1::routing
{
    using httpadv::v1::core::HttpHeaderNames;
    using httpadv::v1::handlers::RouteParameters;

    namespace
    {
        char toLowerAscii(char value)
        {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
        }

        std::string toLowerAsciiString(std::string_view value)
        {
            std::string lowered(value);
            std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch)
                           { return static_cast<char>(std::tolower(ch)); });
            return lowered;
        }

        bool startsWithIgnoreCase(std::string_view value, std::string_view prefix)
        {
            return value.size() >= prefix.size() &&
                   std::equal(prefix.begin(), prefix.end(), value.begin(), [](char lhs, char rhs)
                              { return toLowerAscii(lhs) == toLowerAscii(rhs); });
        }

        bool isRouteParameterNameChar(char value)
        {
            const unsigned char ch = static_cast<unsigned char>(value);
            return std::isalnum(ch) != 0 || value == '_' || value == '-' || value == '.';
        }

        bool isNamedRouteParameter(std::string_view segment)
        {
            if (segment.size() < 2 || segment.front() != REQUEST_MATCHER_PARAMETER_PREFIX)
            {
                return false;
            }

            for (std::size_t index = 1; index < segment.size(); ++index)
            {
                if (!isRouteParameterNameChar(segment[index]))
                {
                    return false;
                }
            }

            return true;
        }

        bool isGlobSegment(std::string_view segment)
        {
            return segment.find(REQUEST_MATCHER_GLOB_WILDCARD_CHAR) != std::string_view::npos;
        }

        bool isGlobStarSegment(std::string_view segment)
        {
            return segment == "**";
        }

        std::string_view trimLeadingDelimiter(std::string_view value)
        {
            if (!value.empty() && value.front() == REQUEST_MATCHER_PATH_DELIMITER)
            {
                return value.substr(1);
            }

            return value;
        }

        std::vector<std::string_view> splitPathSegments(std::string_view value)
        {
            std::vector<std::string_view> segments;
            std::size_t start = 0;

            while (start <= value.size())
            {
                const std::size_t end = value.find(REQUEST_MATCHER_PATH_DELIMITER, start);
                if (end == std::string_view::npos)
                {
                    segments.push_back(value.substr(start));
                    break;
                }

                segments.push_back(value.substr(start, end - start));
                start = end + 1;
            }

            return segments;
        }

        std::string joinPathSegments(const std::vector<std::string_view> &segments, std::size_t beginIndex, std::size_t endIndex)
        {
            std::string joined;
            for (std::size_t index = beginIndex; index < endIndex; ++index)
            {
                if (!joined.empty())
                {
                    joined.push_back(REQUEST_MATCHER_PATH_DELIMITER);
                }
                joined.append(segments[index].data(), segments[index].size());
            }

            return joined;
        }

        bool segmentMatchesGlob(std::string_view segment, std::string_view pattern)
        {
            std::size_t segmentIndex = 0;
            std::size_t patternIndex = 0;
            std::size_t starPatternIndex = std::string_view::npos;
            std::size_t starSegmentIndex = 0;

            while (segmentIndex < segment.size())
            {
                if (patternIndex < pattern.size() && pattern[patternIndex] == REQUEST_MATCHER_GLOB_WILDCARD_CHAR)
                {
                    starPatternIndex = patternIndex++;
                    starSegmentIndex = segmentIndex;
                    continue;
                }

                if (patternIndex < pattern.size() && pattern[patternIndex] == segment[segmentIndex])
                {
                    ++patternIndex;
                    ++segmentIndex;
                    continue;
                }

                if (starPatternIndex != std::string_view::npos)
                {
                    patternIndex = starPatternIndex + 1;
                    segmentIndex = ++starSegmentIndex;
                    continue;
                }

                return false;
            }

            while (patternIndex < pattern.size() && pattern[patternIndex] == REQUEST_MATCHER_GLOB_WILDCARD_CHAR)
            {
                ++patternIndex;
            }

            return patternIndex == pattern.size();
        }

        bool insertParameter(RouteParameters *parameters, std::string key, std::string value)
        {
            if (parameters == nullptr)
            {
                return true;
            }

            const auto inserted = parameters->emplace(std::move(key), std::move(value));
            return inserted.second;
        }

        void eraseParameter(RouteParameters *parameters, const std::string &key)
        {
            if (parameters != nullptr)
            {
                parameters->erase(key);
            }
        }

        bool matchPathSegments(
            const std::vector<std::string_view> &pathSegments,
            const std::vector<std::string_view> &patternSegments,
            std::size_t pathIndex,
            std::size_t patternIndex,
            RouteParameters *parameters,
            std::size_t globOrdinal)
        {
            if (patternIndex == patternSegments.size())
            {
                return pathIndex == pathSegments.size();
            }

            const std::string_view patternSegment = patternSegments[patternIndex];

            if (isGlobStarSegment(patternSegment))
            {
                const std::string captureKey = std::to_string(globOrdinal);
                for (std::size_t consumed = pathIndex; consumed <= pathSegments.size(); ++consumed)
                {
                    const std::string captureValue = joinPathSegments(pathSegments, pathIndex, consumed);
                    if (!insertParameter(parameters, captureKey, captureValue))
                    {
                        return false;
                    }

                    if (matchPathSegments(pathSegments, patternSegments, consumed, patternIndex + 1, parameters, globOrdinal + 1))
                    {
                        return true;
                    }

                    eraseParameter(parameters, captureKey);
                }

                return false;
            }

            if (pathIndex >= pathSegments.size())
            {
                return false;
            }

            const std::string_view pathSegment = pathSegments[pathIndex];

            if (isNamedRouteParameter(patternSegment))
            {
                if (pathSegment.empty())
                {
                    return false;
                }

                const std::string name(patternSegment.substr(1));
                if (!insertParameter(parameters, name, std::string(pathSegment)))
                {
                    return false;
                }

                if (matchPathSegments(pathSegments, patternSegments, pathIndex + 1, patternIndex + 1, parameters, globOrdinal))
                {
                    return true;
                }

                eraseParameter(parameters, name);
                return false;
            }

            if (!patternSegment.empty() && patternSegment.front() == REQUEST_MATCHER_PARAMETER_PREFIX)
            {
                return false;
            }

            if (isGlobSegment(patternSegment))
            {
                if (!segmentMatchesGlob(pathSegment, patternSegment))
                {
                    return false;
                }

                const std::string captureKey = std::to_string(globOrdinal);
                if (!insertParameter(parameters, captureKey, std::string(pathSegment)))
                {
                    return false;
                }

                if (matchPathSegments(pathSegments, patternSegments, pathIndex + 1, patternIndex + 1, parameters, globOrdinal + 1))
                {
                    return true;
                }

                eraseParameter(parameters, captureKey);
                return false;
            }

            if (patternSegment != pathSegment)
            {
                return false;
            }

            return matchPathSegments(pathSegments, patternSegments, pathIndex + 1, patternIndex + 1, parameters, globOrdinal);
        }

        bool matchUriPattern(std::string_view uri, std::string_view uriPattern, RouteParameters *parameters)
        {
            const httpadv::v1::util::UriView parsedUri(uri);
            const std::string_view path = trimLeadingDelimiter(parsedUri.path());
            const std::string_view normalizedPattern = trimLeadingDelimiter(uriPattern);
            const std::vector<std::string_view> pathSegments = splitPathSegments(path);
            const std::vector<std::string_view> patternSegments = splitPathSegments(normalizedPattern);

            if (parameters != nullptr)
            {
                parameters->clear();
            }

            return matchPathSegments(pathSegments, patternSegments, 0, 0, parameters, 0);
        }
    }

    // Default implementations
    bool defaultCheckMethod(std::string_view allowedMethods, std::string_view method)
    {
        if (allowedMethods.empty())
        {
            return true;
        }

        if (method.empty())
        {
            return false;
        }

        return allowedMethods.find(method) != std::string_view::npos;
    }

    bool defaultCheckContentType(HttpRequestContext &context, const std::vector<std::string> &allowedContentTypes)
    {
        std::optional<httpadv::v1::core::HttpHeader> contentType = context.headers().find("Content-Type");
        if (!contentType.has_value())
        {
            return false;
        }

        bool matchFound = false;
        for (const auto &allowedCT : allowedContentTypes)
        {
            if (startsWithIgnoreCase(contentType->valueView(), allowedCT))
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
        return matchUriPattern(uri, uriPattern, nullptr);
    }

    RouteParameters defaultExtractParameters(HttpRequestContext &context, std::string_view uriPattern)
    {
        RouteParameters params;
        matchUriPattern(context.urlView(), uriPattern, &params);
        return params;
    }

    // HandlerMatcher implementations
    HandlerMatcher::HandlerMatcher(std::string_view uriPattern, std::string_view allowedMethods, const std::initializer_list<std::string_view> &allowedContentTypes)
        : uriPattern_(uriPattern), allowedMethods_(allowedMethods),
          methodChecker_(defaultCheckMethod), uriPatternChecker_(defaultCheckUriPattern),
          contentTypeChecker_(defaultCheckContentType), argsExtractor_(defaultExtractParameters)
    {
        setAllowedContentTypes(allowedContentTypes);
        fixStringCases();
    }

    void HandlerMatcher::fixStringCases()
    {
        std::transform(allowedMethods_.begin(), allowedMethods_.end(), allowedMethods_.begin(), [](unsigned char ch)
                       { return static_cast<char>(std::toupper(ch)); });
        for (auto &ct : allowedContentTypes_)
        {
            ct = toLowerAsciiString(ct);
        }
    }

    HandlerMatcher::HandlerMatcher(const char *uriPattern) : HandlerMatcher(std::string_view(uriPattern != nullptr ? uriPattern : "")) {}

    HandlerMatcher::HandlerMatcher(std::string_view uriPattern, MethodChecker methodChecker, UriPatternChecker uriPatternChecker,
                                          ContentTypeChecker contentTypeChecker, ArgsExtractor argsExtractor,
                                          std::string_view allowedMethods, const std::initializer_list<std::string_view> &allowedContentTypes)
        : uriPattern_(uriPattern), allowedMethods_(allowedMethods),
          methodChecker_(methodChecker), uriPatternChecker_(uriPatternChecker),
          contentTypeChecker_(contentTypeChecker), argsExtractor_(argsExtractor)
    {
        setAllowedContentTypes(allowedContentTypes);
        fixStringCases();
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

    void HandlerMatcher::setUriPattern(const char *uriPattern)
    {
        uriPattern_ = uriPattern != nullptr ? uriPattern : "";
    }

    void HandlerMatcher::setUriPattern(std::string_view uriPattern)
    {
        uriPattern_.assign(uriPattern.data(), uriPattern.size());
    }

    void HandlerMatcher::setAllowedMethods(const char *methods)
    {
        allowedMethods_ = methods != nullptr ? methods : "";
        fixStringCases();
    }

    void HandlerMatcher::setAllowedMethods(std::string_view methods)
    {
        allowedMethods_.assign(methods.data(), methods.size());
        fixStringCases();
    }

    void HandlerMatcher::setAllowedContentTypes(const std::initializer_list<const char *> &contentTypes)
    {
        allowedContentTypes_.clear();
        for (const char *contentType : contentTypes)
        {
            allowedContentTypes_.push_back(toLowerAsciiString(contentType != nullptr ? std::string_view(contentType) : std::string_view()));
        }
    }

    void HandlerMatcher::setAllowedContentTypes(const std::initializer_list<std::string_view> &contentTypes)
    {
        allowedContentTypes_.clear();
        for (std::string_view contentType : contentTypes)
        {
            allowedContentTypes_.push_back(toLowerAsciiString(contentType));
        }
    }

    // Getters
    std::string_view HandlerMatcher::getUriPattern() const
    {
        return uriPattern_;
    }

    std::string_view HandlerMatcher::getAllowedMethods() const
    {
        return allowedMethods_;
    }

    const std::vector<std::string> &HandlerMatcher::getAllowedContentTypes() const
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
    bool HandlerMatcher::canHandle(HttpRequestContext &context) const
    {
        if (!allowedMethods_.empty() && !methodChecker_(allowedMethods_, context.methodView()))
        {
            return false;
        }

        if (!uriPatternChecker_(context.urlView(), uriPattern_))
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

    RouteParameters HandlerMatcher::extractParameters(HttpRequestContext &context) const
    {
        return argsExtractor_(context, uriPattern_);
    }

    // ParameterizedUri implementation
    ParameterizedUri::ParameterizedUri(std::string_view uriPattern, std::string_view allowedMethods, const std::initializer_list<std::string_view> &allowedContentTypes)
        : HandlerMatcher(uriPattern, allowedMethods, allowedContentTypes)
    {
    }

} // namespace HttpServerAdvanced


