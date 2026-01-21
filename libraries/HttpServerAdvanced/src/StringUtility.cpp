#include "./StringUtility.h"

namespace HttpServerAdvanced
{
    namespace StringUtil
    {
        int compareTo(const char* a, size_t aLength, const char* b, size_t bLength, bool ignoreCase)
        {
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

        bool startsWith(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, bool ignoreCase)
        {
            if (needleLength > haystackLength)
                return false;
            if (haystackLength == 0 || needleLength == 0)
                return needleLength == 0;
            return compareTo(haystack, needleLength, needle, needleLength, ignoreCase) == 0;
        }

        bool endsWith(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, bool ignoreCase)
        {
            if (needleLength > haystackLength)
                return false;
            if (haystackLength == 0 || needleLength == 0)
                return needleLength == 0;
            return compareTo(haystack + haystackLength - needleLength, needleLength, needle, needleLength, ignoreCase) == 0;
        }

        std::ptrdiff_t indexOf(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, size_t fromIndex, bool ignoreCase)
        {
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

        std::ptrdiff_t lastIndexOf(const char* haystack, size_t haystackLength, const char* needle, size_t needleLength, size_t fromIndex, bool ignoreCase)
        {
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
    }

    
}
