#pragma once
#include <Arduino.h>
#include <cstddef>

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

        // Overloads for Arduino String
        int compareTo(const String& a, const String& b, bool ignoreCase);
        
        bool startsWith(const String& haystack, const String& needle, bool ignoreCase);
        
        bool endsWith(const String& haystack, const String& needle, bool ignoreCase);
        
        std::ptrdiff_t indexOf(const String& haystack, const String& needle, size_t fromIndex = 0, bool ignoreCase = false);
        
        std::ptrdiff_t lastIndexOf(const String& haystack, const String& needle, size_t fromIndex = 0, bool ignoreCase = false);

        String replace(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, const char* replacement, size_t replacementLength, bool ignoreCase);
    }

}