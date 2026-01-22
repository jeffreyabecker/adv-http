#pragma once
#include <Arduino.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
namespace HttpServerAdvanced
{
    namespace StringUtil
    {
        // ========== Implementations ==========

        inline int compareTo(const char* a, size_t aLength, const char* b, size_t bLength, bool ignoreCase)
        {
            if (!a || !b)
                return (!a && !b) ? 0 : (!a ? -1 : 1);
            if (aLength == 0 && bLength == 0)
                return 0;
            if (aLength == 0)
                return -1;
            if (bLength == 0)
                return 1;
            size_t minLen = std::min(aLength, bLength);
            const char* aData = a;
            const char* bData = b;
            for (size_t i = 0; i < minLen; ++i)
            {
                char charA = aData[i];
                char charB = bData[i];
                if (ignoreCase)
                {
                    charA = std::tolower(static_cast<unsigned char>(charA));
                    charB = std::tolower(static_cast<unsigned char>(charB));
                }
                if (charA < charB)
                    return -1;
                if (charA > charB)
                    return 1;
            }
            if (aLength < bLength)
                return -1;
            if (aLength > bLength)
                return 1;
            return 0;
        }

        inline bool startsWith(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, bool ignoreCase)
        {
            if (!haystack || !needle)
                return false;
            if (needleLength > haystackLength)
                return false;
            if (haystackLength == 0 || needleLength == 0)
                return needleLength == 0;
            return compareTo(haystack, needleLength, needle, needleLength, ignoreCase) == 0;
        }

        inline bool endsWith(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, bool ignoreCase)
        {
            if (!haystack || !needle)
                return false;
            if (needleLength > haystackLength)
                return false;
            if (haystackLength == 0 || needleLength == 0)
                return needleLength == 0;
            return compareTo(haystack + haystackLength - needleLength, needleLength, needle, needleLength, ignoreCase) == 0;
        }

        inline std::ptrdiff_t indexOf(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false)
        {
            if (!haystack || !needle)
                return -1;
            if (fromIndex >= haystackLength || needleLength == 0)
                return -1;
            if (haystackLength == 0 || needleLength == 0)
                return -1;

            const char* hData = haystack;
            const char* nData = needle;
            if (ignoreCase)
            {
                auto it = std::search(hData + fromIndex, hData + haystackLength,
                                      nData, nData + needleLength,
                                      [](char a, char b)
                                      {
                                          return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
                                      });
                if (it != hData + haystackLength)
                {
                    return std::distance(hData, it);
                }
            }
            else
            {
                auto it = std::search(hData + fromIndex, hData + haystackLength,
                                      nData, nData + needleLength);
                if (it != hData + haystackLength)
                {
                    return std::distance(hData, it);
                }
            }
            return -1;
        }

        inline std::ptrdiff_t lastIndexOf(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false)
        {
            if (!haystack || !needle)
                return -1;
            if (fromIndex >= haystackLength || needleLength == 0)
                return -1;
            if (haystackLength == 0 || needleLength == 0)
                return -1;

            size_t searchEnd = (fromIndex >= haystackLength) ? haystackLength : fromIndex + 1;
            if (searchEnd < needleLength)
                return -1;

            const char* hData = haystack;
            auto searchStart = hData;
            auto searchEndPtr = hData + searchEnd;

            if (ignoreCase)
            {
                auto it = std::find_end(searchStart, searchEndPtr,
                                        needle, needle + needleLength,
                                        [](char a, char b)
                                        {
                                            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
                                        });
                if (it != searchEndPtr)
                {
                    return std::distance(hData, it);
                }
            }
            else
            {
                auto it = std::find_end(searchStart, searchEndPtr,
                                        needle, needle + needleLength);
                if (it != searchEndPtr)
                {
                    return std::distance(hData, it);
                }
            }
            return -1;
        }

        // Overloads for Arduino String
        inline int compareTo(const String& a, const String& b, bool ignoreCase){
            return compareTo(a.c_str(), a.length(), b.c_str(), b.length(), ignoreCase);
        }
        inline bool startsWith(const String& haystack, const String& needle, bool ignoreCase){
            return startsWith(haystack.c_str(), haystack.length(), needle.c_str(), needle.length(), ignoreCase);
        }
        inline bool endsWith(const String& haystack, const String& needle, bool ignoreCase){
            return endsWith(haystack.c_str(), haystack.length(), needle.c_str(), needle.length(), ignoreCase);
        }
        inline std::ptrdiff_t indexOf(const String& haystack, const String& needle, size_t fromIndex = 0, bool ignoreCase = false){
            return indexOf(haystack.c_str(), haystack.length(), needle.c_str(), needle.length(), fromIndex, ignoreCase);
        }
        inline std::ptrdiff_t lastIndexOf(const String& haystack, const String& needle, size_t fromIndex = 0, bool ignoreCase = false){
            return lastIndexOf(haystack.c_str(), haystack.length(), needle.c_str(), needle.length(), fromIndex, ignoreCase);
        }

        inline String replace(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, const char* replacement, size_t replacementLength, bool ignoreCase){

            if (!haystack || !needle || !replacement)
                return String();
            if (haystackLength == 0 || needleLength == 0)
                return String(haystack, haystackLength);

            String result;
            size_t pos = 0;
            while (pos < haystackLength)
            {
                std::ptrdiff_t index = indexOf(haystack, haystackLength, needle, needleLength, pos, ignoreCase);
                if (index == -1)
                {
                    result += String(haystack + pos, haystackLength - pos);
                    break;
                }
                result += String(haystack + pos, index - pos);
                result += String(replacement, replacementLength);
                pos = index + needleLength;
            }
            return result;

        }
    }

}