#pragma once
#include <Arduino.h>
#include "StringUtility.h"
#include <cstring>
namespace HttpServerAdvanced
{
    class StringView
    {
    public:
        // Constructors
        StringView() : _data(nullptr), _length(0) {}
        StringView(const char *cstr, size_t length) : _data(cstr), _length(length) {}
        StringView(const StringView &other) : _data(other._data), _length(other._length) {}
        StringView(const char *cstr) : _data(cstr), _length(cstr ? strlen(cstr) : 0) {}
        StringView(const String &str) : _data(str.c_str()), _length(str.length()) {}

        virtual ~StringView() = default;

        // Comparison methods
        int compareTo(const char *cstr, size_t length, bool ignoreCase = false) const
        {
            return StringUtil::compareTo(_data, _length, cstr, length, ignoreCase);
        }
        int compareTo(const char *cstr, bool ignoreCase) const
        {
            return StringUtil::compareTo(_data, _length, cstr, strlen(cstr), ignoreCase);
        }
        int compareTo(const String &s, bool ignoreCase) const
        {
            return StringUtil::compareTo(_data, _length, s.c_str(), s.length(), ignoreCase);
        }
        int compareTo(const StringView &s, bool ignoreCase) const
        {
            return StringUtil::compareTo(_data, _length, s._data, s._length, ignoreCase);
        }

        // Character access methods
        char charAt(size_t index) const
        {
            if (index >= _length || _data == nullptr)
            {
                return '\0';
            }
            return _data[index];
        }
        char operator[](size_t index) const
        {
            return charAt(index);
        }

        // Equality methods
        bool equals(const char *cstr, size_t length, bool ignoreCase = false) const
        {
            return compareTo(cstr, length, ignoreCase) == 0;
        }
        bool equals(const char *cstr, bool ignoreCase = false) const
        {
            return compareTo(cstr, ignoreCase) == 0;
        }
        bool equals(const StringView &s, bool ignoreCase = false) const
        {
            return compareTo(s, ignoreCase) == 0;
        }

        bool operator==(const StringView &other) const
        {
            return equals(other);
        }
        bool operator!=(const StringView &other) const
        {
            return !equals(other);
        }
        bool operator==(const char *cstr) const
        {
            return equals(cstr);
        }
        bool operator!=(const char *cstr) const
        {
            return !equals(cstr);
        }
        bool operator==(const String &str) const
        {
            return equals(str);
        }
        bool operator!=(const String &str) const
        {
            return !equals(str);
        }

        // Iterator methods
        const char *begin() const
        {
            return _data;
        }
        const char *end() const
        {
            if (!_data)
            {
                return nullptr;
            }
            return _data + _length;
        }

        // Query methods
        bool isEmpty() const
        {
            return _length == 0 || _data == nullptr;
        }
        const char *c_str() const
        {
            return _data;
        }
        size_t length() const
        {
            return _length;
        }

        // Search methods
        std::ptrdiff_t indexOf(const StringView &needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::indexOf(_data, _length, needle._data, needle._length, fromIndex, ignoreCase);
        }
        std::ptrdiff_t indexOf(const char *needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::indexOf(_data, _length, needle, needleLength, fromIndex, ignoreCase);
        }
        std::ptrdiff_t indexOf(const char *needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::indexOf(_data, _length, needle, strlen(needle), fromIndex, ignoreCase);
        }
        std::ptrdiff_t indexOf(const String &needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::indexOf(_data, _length, needle.c_str(), needle.length(), fromIndex, ignoreCase);
        }
        std::ptrdiff_t indexOf(char needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            char needleStr[2] = {needle, '\0'};
            return StringUtil::indexOf(_data, _length, needleStr, 1, fromIndex, ignoreCase);
        }

        std::ptrdiff_t lastIndexOf(const StringView &needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(_data, _length, needle._data, needle._length, fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(const char *needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(_data, _length, needle, needleLength, fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(const char *needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(_data, _length, needle, strlen(needle), fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(const String &needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(_data, _length, needle.c_str(), needle.length(), fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(char needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            char needleStr[2] = {needle, '\0'};
            return StringUtil::lastIndexOf(_data, _length, needleStr, 1, fromIndex, ignoreCase);
        }

        // Substring methods
        bool startsWith(const StringView &prefix, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(_data, _length, prefix._data, prefix._length, ignoreCase);
        }
        bool startsWith(const char *prefix, size_t prefixLength, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(_data, _length, prefix, prefixLength, ignoreCase);
        }
        bool startsWith(const char *prefix, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(_data, _length, prefix, strlen(prefix), ignoreCase);
        }
        bool startsWith(const String &prefix, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(_data, _length, prefix.c_str(), prefix.length(), ignoreCase);
        }
        bool startsWith(char prefix, bool ignoreCase = false) const
        {
            char prefixStr[2] = {prefix, '\0'};
            return StringUtil::startsWith(_data, _length, prefixStr, 1, ignoreCase);
        }

        bool endsWith(const StringView &suffix, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(_data, _length, suffix._data, suffix._length, ignoreCase);
        }
        bool endsWith(const char *suffix, size_t suffixLength, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(_data, _length, suffix, suffixLength, ignoreCase);
        }
        bool endsWith(const char *suffix, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(_data, _length, suffix, strlen(suffix), ignoreCase);
        }
        bool endsWith(const String &suffix, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(_data, _length, suffix.c_str(), suffix.length(), ignoreCase);
        }
        bool endsWith(char suffix, bool ignoreCase = false) const
        {
            char suffixStr[2] = {suffix, '\0'};
            return StringUtil::endsWith(_data, _length, suffixStr, 1, ignoreCase);
        }

        StringView substring(size_t beginIndex) const
        {
            if (beginIndex >= _length)
                return StringView();
            return StringView(_data + beginIndex, _length - beginIndex);
        }
        StringView substring(size_t beginIndex, size_t endIndex) const
        {
            if (beginIndex >= _length || endIndex <= beginIndex)
                return StringView();
            size_t subLength = (endIndex > _length ? _length : endIndex) - beginIndex;
            return StringView(_data + beginIndex, subLength);
        }
        StringView trim() const
        {
            size_t start = 0;
            size_t end = _length;
            while (start < end && isspace(_data[start]))
                start++;
            while (end > start && isspace(_data[end - 1]))
                end--;
            return StringView(_data + start, end - start);
        }

        // Conversion methods
        String toString() const
        {
            return String(_data);
        }
        String toLowerCase() const
        {
            String result = toString();
            result.toLowerCase();
            return result;
        }
        String toUpperCase() const
        {
            String result = toString();
            result.toUpperCase();
            return result;
        }
        String replace(char oldChar, char newChar, bool ignoreCase = false) const
        {
            return StringUtil::replace(_data, _length, &oldChar, 1, &newChar, 1, ignoreCase);
        }
        String replace(const StringView &needle, const StringView &newStr, bool ignoreCase = false) const
        {
            return StringUtil::replace(_data, _length, needle._data, needle._length, newStr._data, newStr._length, ignoreCase);
        }

    protected:
        const char *_data;
        size_t _length;
    };
    // a specialization of StringView that *might* own the underlying string data
    class OwningStringView : public StringView
    {
    private:
        String ownedString_;

    public:
        // Constructors
        OwningStringView(const char *cstr, size_t length, bool copyValue = true) 
            : ownedString_(copyValue ? String(cstr, length) : String()), 
              StringView(copyValue ? ownedString_.c_str() : cstr, copyValue ? ownedString_.length() : length) 
        {}
        OwningStringView(const StringView &other, bool copyValue = true) 
            : ownedString_(copyValue ? String(other.c_str(), other.length()) : String()), 
              StringView(copyValue ? ownedString_.c_str() : other.c_str(), copyValue ? ownedString_.length() : other.length()) 
            {}
        OwningStringView(const char *cstr, bool copyValue = true) 
            : ownedString_(copyValue ? String(cstr) : String()), 
              StringView(copyValue ? ownedString_.c_str() : cstr, copyValue ? ownedString_.length() : strlen(cstr)) 
        {}
        OwningStringView(const String &str, bool copyValue = true) 
            : ownedString_(copyValue ? str : String()), 
              StringView(copyValue ? ownedString_.c_str() : str.c_str(), copyValue ? ownedString_.length() : str.length()) 
        {}
        // we always own the data if the input is an rvalue String
        OwningStringView(String &&str) : ownedString_(std::move(str)), StringView(ownedString_.c_str(), ownedString_.length()) {}
    };
} // namespace HttpServerAdvanced