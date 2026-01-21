#pragma once
#include <Arduino.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
namespace HttpServerAdvanced
{
    namespace StringUtil
    {
        int compareTo(const char* a, size_t aLength, const char* b, size_t bLength, bool ignoreCase);
        bool startsWith(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, bool ignoreCase);
        bool endsWith(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, bool ignoreCase);
        std::ptrdiff_t indexOf(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false);        
        std::ptrdiff_t lastIndexOf(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false);

        // Overloads for Arduino String
        int compareTo(const String& a, const String& b, bool ignoreCase){
            return compareTo(a.c_str(), a.length(), b.c_str(), b.length(), ignoreCase);
        }
        bool startsWith(const String& haystack, const String& needle, bool ignoreCase){
            return startsWith(haystack.c_str(), haystack.length(), needle.c_str(), needle.length(), ignoreCase);
        }
        bool endsWith(const String& haystack, const String& needle, bool ignoreCase){
            return endsWith(haystack.c_str(), haystack.length(), needle.c_str(), needle.length(), ignoreCase);
        }
        std::ptrdiff_t indexOf(const String& haystack, const String& needle, size_t fromIndex = 0, bool ignoreCase = false){
            return indexOf(haystack.c_str(), haystack.length(), needle.c_str(), needle.length(), fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(const String& haystack, const String& needle, size_t fromIndex = 0, bool ignoreCase = false){
            return lastIndexOf(haystack.c_str(), haystack.length(), needle.c_str(), needle.length(), fromIndex, ignoreCase);
        }
    }

}