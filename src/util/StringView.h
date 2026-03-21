#pragma once

#include <Arduino.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>

#include "StringUtility.h"

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
        StringView(std::string_view view) : _data(view.data()), _length(view.size()) {}

        virtual ~StringView() = default;

        std::string_view stdView() const
        {
            return _data == nullptr ? std::string_view() : std::string_view(_data, _length);
        }

        // Comparison methods
        int compareTo(const char *cstr, size_t length, bool ignoreCase = false) const
        {
            return StringUtil::compareTo(stdView(), std::string_view(cstr, length), ignoreCase);
        }
        int compareTo(const char *cstr, bool ignoreCase) const
        {
            return StringUtil::compareTo(stdView(), std::string_view(cstr, strlen(cstr)), ignoreCase);
        }
        int compareTo(const String &s, bool ignoreCase) const
        {
            return StringUtil::compareTo(stdView(), std::string_view(s.c_str(), s.length()), ignoreCase);
        }
        int compareTo(const StringView &s, bool ignoreCase) const
        {
            return StringUtil::compareTo(stdView(), s.stdView(), ignoreCase);
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
            return StringUtil::indexOf(stdView(), needle.stdView(), fromIndex, ignoreCase);
        }
        std::ptrdiff_t indexOf(const char *needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::indexOf(stdView(), std::string_view(needle, needleLength), fromIndex, ignoreCase);
        }
        std::ptrdiff_t indexOf(const char *needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::indexOf(stdView(), std::string_view(needle, strlen(needle)), fromIndex, ignoreCase);
        }
        std::ptrdiff_t indexOf(const String &needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::indexOf(stdView(), std::string_view(needle.c_str(), needle.length()), fromIndex, ignoreCase);
        }
        std::ptrdiff_t indexOf(char needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::indexOf(stdView(), std::string_view(&needle, 1), fromIndex, ignoreCase);
        }

        std::ptrdiff_t lastIndexOf(const StringView &needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(stdView(), needle.stdView(), fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(const char *needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(stdView(), std::string_view(needle, needleLength), fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(const char *needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(stdView(), std::string_view(needle, strlen(needle)), fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(const String &needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(stdView(), std::string_view(needle.c_str(), needle.length()), fromIndex, ignoreCase);
        }
        std::ptrdiff_t lastIndexOf(char needle, size_t fromIndex = 0, bool ignoreCase = false) const
        {
            return StringUtil::lastIndexOf(stdView(), std::string_view(&needle, 1), fromIndex, ignoreCase);
        }

        // Substring methods
        bool startsWith(const StringView &prefix, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(stdView(), prefix.stdView(), ignoreCase);
        }
        bool startsWith(const char *prefix, size_t prefixLength, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(stdView(), std::string_view(prefix, prefixLength), ignoreCase);
        }
        bool startsWith(const char *prefix, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(stdView(), std::string_view(prefix, strlen(prefix)), ignoreCase);
        }
        bool startsWith(const String &prefix, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(stdView(), std::string_view(prefix.c_str(), prefix.length()), ignoreCase);
        }
        bool startsWith(char prefix, bool ignoreCase = false) const
        {
            return StringUtil::startsWith(stdView(), std::string_view(&prefix, 1), ignoreCase);
        }

        bool endsWith(const StringView &suffix, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(stdView(), suffix.stdView(), ignoreCase);
        }
        bool endsWith(const char *suffix, size_t suffixLength, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(stdView(), std::string_view(suffix, suffixLength), ignoreCase);
        }
        bool endsWith(const char *suffix, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(stdView(), std::string_view(suffix, strlen(suffix)), ignoreCase);
        }
        bool endsWith(const String &suffix, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(stdView(), std::string_view(suffix.c_str(), suffix.length()), ignoreCase);
        }
        bool endsWith(char suffix, bool ignoreCase = false) const
        {
            return StringUtil::endsWith(stdView(), std::string_view(&suffix, 1), ignoreCase);
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
            std::string_view view = stdView();
            size_t start = 0;
            size_t end = view.size();
            while (start < end && std::isspace(static_cast<unsigned char>(view[start])))
                start++;
            while (end > start && std::isspace(static_cast<unsigned char>(view[end - 1])))
                end--;
            return StringView(view.substr(start, end - start));
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
                std::string ownedString_;

    public:
        // Constructors
        OwningStringView(const char *cstr, size_t length, bool copyValue = true) 
                        : ownedString_(copyValue ? std::string(cstr != nullptr ? cstr : "", length) : std::string()), 
                            StringView(copyValue ? ownedString_.c_str() : cstr, copyValue ? ownedString_.size() : length) 
        {}
        OwningStringView(const StringView &other, bool copyValue = true) 
                        : ownedString_(copyValue ? std::string(other.c_str() != nullptr ? other.c_str() : "", other.length()) : std::string()), 
                            StringView(copyValue ? ownedString_.c_str() : other.c_str(), copyValue ? ownedString_.size() : other.length()) 
            {}
        OwningStringView(const char *cstr, bool copyValue = true) 
                        : ownedString_(copyValue ? std::string(cstr != nullptr ? cstr : "") : std::string()), 
                            StringView(copyValue ? ownedString_.c_str() : cstr, copyValue ? ownedString_.size() : (cstr ? strlen(cstr) : 0)) 
        {}
        OwningStringView(const String &str, bool copyValue = true) 
                        : ownedString_(copyValue ? std::string(str.c_str(), str.length()) : std::string()), 
                            StringView(copyValue ? ownedString_.c_str() : str.c_str(), copyValue ? ownedString_.size() : str.length()) 
        {}
        // we always own the data if the input is an rvalue String
                OwningStringView(String &&str) : ownedString_(str.c_str(), str.length()), StringView(ownedString_.c_str(), ownedString_.size()) {}
    };
} // namespace HttpServerAdvanced