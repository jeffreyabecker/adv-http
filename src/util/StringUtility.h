#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace HttpServerAdvanced
{
    namespace StringUtil
    {
        // ========== Declarations ==========

        int compareTo(const char* a, size_t aLength, const char* b, size_t bLength, bool ignoreCase);

        bool startsWith(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, bool ignoreCase);

        bool endsWith(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, bool ignoreCase);

        std::ptrdiff_t indexOf(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false);

        std::ptrdiff_t lastIndexOf(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false);

        int compareTo(std::string_view a, std::string_view b, bool ignoreCase = false);

        bool startsWith(std::string_view haystack, std::string_view needle, bool ignoreCase = false);

        bool endsWith(std::string_view haystack, std::string_view needle, bool ignoreCase = false);

        std::ptrdiff_t indexOf(std::string_view haystack, std::string_view needle, size_t fromIndex = 0, bool ignoreCase = false);

        std::ptrdiff_t lastIndexOf(std::string_view haystack, std::string_view needle, size_t fromIndex = 0, bool ignoreCase = false);

        std::string replace(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, const char* replacement, size_t replacementLength, bool ignoreCase);

        std::string replace(std::string_view haystack, std::string_view needle, std::string_view replacement, bool ignoreCase = false);
    }

}