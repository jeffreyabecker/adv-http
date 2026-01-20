#include "StringView.h"

namespace HTTPServer
{
    namespace StringUtil
    {
        int compareTo(const StringView &a, const StringView &b, bool ignoreCase)
        {
            if (a.isEmpty() && b.isEmpty())
                return 0;
            if (a.isEmpty())
                return -1;
            if (b.isEmpty())
                return 1;
            size_t minLen = std::min(a.length(), b.length());
            const char* aData = a.begin();
            const char* bData = b.begin();
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
            if (a.length() < b.length())
                return -1;
            if (a.length() > b.length())
                return 1;
            return 0;
        }

        bool startsWith(const StringView &haystack, const StringView &needle, bool ignoreCase)
        {
            if (needle.length() > haystack.length())
                return false;
            if (haystack.isEmpty() || needle.isEmpty())
                return needle.isEmpty();
            return compareTo(StringView(haystack.begin(), needle.length()), needle, ignoreCase) == 0;
        }

        bool endsWith(const StringView &haystack, const StringView &needle, bool ignoreCase)
        {
            if (needle.length() > haystack.length())
                return false;
            if (haystack.isEmpty() || needle.isEmpty())
                return needle.isEmpty();
            return compareTo(StringView(haystack.begin() + haystack.length() - needle.length(), needle.length()), needle, ignoreCase) == 0;
        }

        std::ptrdiff_t indexOf(const StringView &haystack, const StringView &needle, size_t fromIndex, bool ignoreCase)
        {
            if (fromIndex >= haystack.length() || needle.isEmpty())
                return -1;
            if (haystack.isEmpty() || needle.isEmpty())
                return -1;

            const char* hData = haystack.begin();
            const char* nData = needle.begin();
            if (ignoreCase)
            {
                auto it = std::search(hData + fromIndex, hData + haystack.length(),
                                      nData, nData + needle.length(),
                                      [](char a, char b)
                                      {
                                          return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
                                      });
                if (it != hData + haystack.length())
                {
                    return std::distance(hData, it);
                }
            }
            else
            {
                auto it = std::search(hData + fromIndex, hData + haystack.length(),
                                      nData, nData + needle.length());
                if (it != hData + haystack.length())
                {
                    return std::distance(hData, it);
                }
            }
            return -1;
        }

        std::ptrdiff_t lastIndexOf(const StringView &haystack, const StringView &needle, size_t fromIndex, bool ignoreCase)
        {
            if (fromIndex >= haystack.length() || needle.isEmpty())
                return -1;
            if (haystack.isEmpty() || needle.isEmpty())
                return -1;

            size_t searchEnd = (fromIndex >= haystack.length()) ? haystack.length() : fromIndex + 1;
            if (searchEnd < needle.length())
                return -1;

            const char* hData = haystack.begin();
            auto searchStart = hData;
            auto searchEndPtr = hData + searchEnd;

            if (ignoreCase)
            {
                auto it = std::find_end(searchStart, searchEndPtr,
                                        needle.begin(), needle.begin() + needle.length(),
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
                                        needle.begin(), needle.begin() + needle.length());
                if (it != searchEndPtr)
                {
                    return std::distance(hData, it);
                }
            }
            return -1;
        }
    }

    // StringView method implementations

    // Constructors
    StringView::StringView() : _data(nullptr), _length(0) {}
    StringView::StringView(const char *cstr, size_t length) : _data(cstr), _length(length) {}
    StringView::StringView(const StringView &other) : _data(other._data), _length(other._length) {}
    StringView::StringView(const String &str) : _data(str.isEmpty() ? nullptr : str.c_str()), _length(str.length()) {}

    // Character access
    char StringView::charAt(size_t index) const
    {
        return (index < _length && _data) ? _data[index] : '\0';
    }

    char StringView::operator[](size_t index) const
    {
        return charAt(index);
    }

    // Comparison methods
    int StringView::compareTo(const char *cstr, size_t length, bool ignoreCase) const
    {
        return StringUtil::compareTo(*this, StringView(cstr, length), ignoreCase);
    }

    int StringView::compareTo(const char *cstr, bool ignoreCase) const
    {
        return compareTo(cstr, cstr ? strlen(cstr) : 0, ignoreCase);
    }

    int StringView::compareTo(const String &s, bool ignoreCase) const
    {
        return compareTo(s.c_str(), s.length(), ignoreCase);
    }

    int StringView::compareTo(const StringView &s, bool ignoreCase) const
    {
        return compareTo(s._data, s._length, ignoreCase);
    }

    // Equality methods
    bool StringView::equals(const char *cstr, size_t length, bool ignoreCase) const
    {
        return compareTo(cstr, length, ignoreCase) == 0;
    }

    bool StringView::equals(const char *cstr, bool ignoreCase) const
    {
        return equals(cstr, cstr ? strlen(cstr) : 0, ignoreCase);
    }

    bool StringView::equals(const StringView &s, bool ignoreCase) const
    {
        return equals(s._data, s._length, ignoreCase);
    }

    bool StringView::operator==(const StringView &other) const
    {
        return equals(other);
    }

    bool StringView::operator!=(const StringView &other) const
    {
        return !equals(other);
    }

    bool StringView::operator==(const char *cstr) const
    {
        return equals(cstr);
    }

    bool StringView::operator!=(const char *cstr) const
    {
        return !equals(cstr);
    }

    bool StringView::operator==(const String &str) const
    {
        return equals(str.c_str(), str.length());
    }

    bool StringView::operator!=(const String &str) const
    {
        return !equals(str.c_str(), str.length());
    }

    // Iterator methods
    const char *StringView::begin() const
    {
        return _data;
    }

    const char *StringView::end() const
    {
        return _data ? _data + _length : nullptr;
    }

    // Query methods
    bool StringView::isEmpty() const
    {
        return _length == 0;
    }

    size_t StringView::length() const
    {
        return _length;
    }

    // Search methods
    std::ptrdiff_t StringView::indexOf(const StringView &needle, size_t fromIndex, bool ignoreCase) const
    {
        return StringUtil::indexOf(*this, needle, fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::indexOf(const char *needle, size_t needleLength, size_t fromIndex, bool ignoreCase) const
    {
        return indexOf(StringView(needle, needleLength), fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::indexOf(const char *needle, size_t fromIndex, bool ignoreCase) const
    {
        return indexOf(StringView(needle), fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::indexOf(const String &needle, size_t fromIndex, bool ignoreCase) const
    {
        return indexOf(StringView(needle), fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::indexOf(char needle, size_t fromIndex, bool ignoreCase) const
    {
        return indexOf(StringView(&needle, 1), fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::lastIndexOf(const StringView &needle, size_t fromIndex, bool ignoreCase) const
    {
        return StringUtil::lastIndexOf(*this, needle, fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::lastIndexOf(const char *needle, size_t needleLength, size_t fromIndex, bool ignoreCase) const
    {
        return lastIndexOf(StringView(needle, needleLength), fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::lastIndexOf(const char *needle, size_t fromIndex, bool ignoreCase) const
    {
        return lastIndexOf(StringView(needle), fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::lastIndexOf(const String &needle, size_t fromIndex, bool ignoreCase) const
    {
        return lastIndexOf(StringView(needle), fromIndex, ignoreCase);
    }

    std::ptrdiff_t StringView::lastIndexOf(char needle, size_t fromIndex, bool ignoreCase) const
    {
        return lastIndexOf(StringView(&needle, 1), fromIndex, ignoreCase);
    }

    // Substring methods
    bool StringView::startsWith(const StringView &prefix, bool ignoreCase) const
    {
        return StringUtil::startsWith(*this, prefix, ignoreCase);
    }

    bool StringView::startsWith(const char *prefix, size_t prefixLength, bool ignoreCase) const
    {
        return startsWith(StringView(prefix, prefixLength), ignoreCase);
    }

    bool StringView::startsWith(const char *prefix, bool ignoreCase) const
    {
        return startsWith(StringView(prefix), ignoreCase);
    }

    bool StringView::startsWith(const String &prefix, bool ignoreCase) const
    {
        return startsWith(StringView(prefix), ignoreCase);
    }

    bool StringView::startsWith(char prefix, bool ignoreCase) const
    {
        return startsWith(StringView(&prefix, 1), ignoreCase);
    }

    bool StringView::endsWith(const StringView &suffix, bool ignoreCase) const
    {
        return StringUtil::endsWith(*this, suffix, ignoreCase);
    }

    bool StringView::endsWith(const char *suffix, size_t suffixLength, bool ignoreCase) const
    {
        return endsWith(StringView(suffix, suffixLength), ignoreCase);
    }

    bool StringView::endsWith(const char *suffix, bool ignoreCase) const
    {
        return endsWith(StringView(suffix), ignoreCase);
    }

    bool StringView::endsWith(const String &suffix, bool ignoreCase) const
    {
        return endsWith(StringView(suffix), ignoreCase);
    }

    bool StringView::endsWith(char suffix, bool ignoreCase) const
    {
        return endsWith(StringView(&suffix, 1), ignoreCase);
    }
    StringView StringView::substring(size_t beginIndex) const
    {
        return substring(beginIndex, _length);
    }

    StringView StringView::substring(size_t beginIndex, size_t endIndex) const
    {
        if (beginIndex >= _length || beginIndex >= endIndex || !_data)
        {
            return StringView(nullptr, 0);
        }

        size_t actualEnd = (endIndex > _length) ? _length : endIndex;
        return StringView(_data + beginIndex, actualEnd - beginIndex);
    }

    StringView StringView::trim() const
    {
        if (!_data || _length == 0)
            return *this;
        auto start = std::find_if(_data, _data + _length, [](char c)
                                  { return !::isspace(static_cast<unsigned char>(c)); });

        if (start == _data + _length)
            return StringView(nullptr, 0);
        auto end = std::find_if(std::reverse_iterator<const char *>(_data + _length),
                                std::reverse_iterator<const char *>(start),
                                [](char c)
                                {
                                    return !::isspace(static_cast<unsigned char>(c));
                                })
                       .base();

        return StringView(start, static_cast<size_t>(std::distance(start, end)));
    }

    // Conversion methods
    String StringView::toString() const
    {
        String result;
        if (!_data)
            return String();
        result.reserve(_length);
        result.concat(_data, _length);
        return result;
    }

    String StringView::toLowerCase() const
    {
        String result;
        if (!_data)
            return String();
        result.reserve(_length);
        for (size_t i = 0; i < _length; ++i)
        {
            result += std::tolower(static_cast<unsigned char>(_data[i]));
        }
        return result;
    }

    String StringView::toUpperCase() const
    {
        String result;
        if (!_data)
            return String();
        result.reserve(_length);
        for (size_t i = 0; i < _length; ++i)
        {
            result += std::toupper(static_cast<unsigned char>(_data[i]));
        }
        return result;
    }

    String StringView::replace(char oldChar, char newChar) const
    {
        String result;
        if (!_data)
            return String();
        result.reserve(_length);
        for (size_t i = 0; i < _length; ++i)
        {
            char c = _data[i];
            if (c == oldChar)
                result += newChar;
            else
                result += c;
        }
        return result;
    }

    String StringView::replace(const StringView &oldStr, const StringView &newStr) const
    {
        String result;
        if (!_data || oldStr.isEmpty())
            return toString();

        size_t pos = 0;
        while (pos < _length)
        {
            std::ptrdiff_t index = indexOf(oldStr, pos);
            if (index == -1)
            {
                result.concat(_data + pos, _length - pos);
                break;
            }
            result.concat(_data + pos, index - pos);
            result.concat(oldStr._data, oldStr._length);
            pos = index + oldStr._length;
        }
        return result;
    }
}
