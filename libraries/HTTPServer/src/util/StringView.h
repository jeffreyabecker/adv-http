#pragma once
#include <Arduino.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
namespace HTTPServer
{
    namespace StringUtil
    {
        int compareTo(const StringView &a, const StringView &b, bool ignoreCase);
        bool startsWith(const StringView &haystack, const StringView &needle, bool ignoreCase);
        bool endsWith(const StringView &haystack, const StringView &needle, bool ignoreCase);
        std::ptrdiff_t indexOf(const StringView &haystack, const StringView &needle, size_t fromIndex = 0, bool ignoreCase = false);
        std::ptrdiff_t lastIndexOf(const StringView &haystack, const StringView &needle, size_t fromIndex = 0, bool ignoreCase = false);
    }
    class StringView
    {
    private:


    public:
        // Constructors
        StringView();
        StringView(const char *cstr, size_t length);
        StringView(const StringView &other);
        constexpr StringView(const char *cstr) : _data(cstr), _length(cstr ? strlen(cstr) : 0) {}

        StringView(const String &str);

        // Comparison methods
        int compareTo(const char *cstr, size_t length, bool ignoreCase = false) const;
        int compareTo(const char *cstr, bool ignoreCase) const;
        int compareTo(const String &s, bool ignoreCase) const;
        int compareTo(const StringView &s, bool ignoreCase) const;

        // Character access methods
        char charAt(size_t index) const;
        char operator[](size_t index) const;

        // Equality methods
        bool equals(const char *cstr, size_t length, bool ignoreCase = false) const;
        bool equals(const char *cstr, bool ignoreCase = false) const;
        bool equals(const StringView &s, bool ignoreCase = false) const;

        bool operator==(const StringView &other) const;
        bool operator!=(const StringView &other) const;
        bool operator==(const char *cstr) const;
        bool operator!=(const char *cstr) const;
        bool operator==(const String &str) const;
        bool operator!=(const String &str) const;

        // Iterator methods
        const char *begin() const;
        const char *end() const;

        // Query methods
        bool isEmpty() const;
        size_t length() const;

        // Search methods
        std::ptrdiff_t indexOf(const StringView &needle, size_t fromIndex = 0, bool ignoreCase = false) const;
        std::ptrdiff_t indexOf(const char *needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false) const;
        std::ptrdiff_t indexOf(const char *needle, size_t fromIndex = 0, bool ignoreCase = false) const;
        std::ptrdiff_t indexOf(const String &needle, size_t fromIndex = 0, bool ignoreCase = false) const;
        std::ptrdiff_t indexOf(char needle, size_t fromIndex = 0, bool ignoreCase = false) const;

        std::ptrdiff_t lastIndexOf(const StringView &needle, size_t fromIndex = 0, bool ignoreCase = false) const;
        std::ptrdiff_t lastIndexOf(const char *needle, size_t needleLength, size_t fromIndex = 0, bool ignoreCase = false) const;
        std::ptrdiff_t lastIndexOf(const char *needle, size_t fromIndex = 0, bool ignoreCase = false) const;
        std::ptrdiff_t lastIndexOf(const String &needle, size_t fromIndex = 0, bool ignoreCase = false) const;
        std::ptrdiff_t lastIndexOf(char needle, size_t fromIndex = 0, bool ignoreCase = false) const;

        // Substring methods

        bool startsWith(const StringView &prefix, bool ignoreCase = false) const;
        bool startsWith(const char *prefix, size_t prefixLength, bool ignoreCase = false) const;
        bool startsWith(const char *prefix, bool ignoreCase = false) const;
        bool startsWith(const String &prefix, bool ignoreCase = false) const;
        bool startsWith(char prefix, bool ignoreCase = false) const;

        bool endsWith(const StringView &suffix, bool ignoreCase = false) const;
        bool endsWith(const char *suffix, size_t suffixLength, bool ignoreCase = false) const;
        bool endsWith(const char *suffix, bool ignoreCase = false) const;
        bool endsWith(const String &suffix, bool ignoreCase = false) const;
        bool endsWith(char suffix, bool ignoreCase = false) const;

        StringView substring(size_t beginIndex) const;
        StringView substring(size_t beginIndex, size_t endIndex) const;
        StringView trim() const;

        // Conversion methods
        String toString() const;
        String toLowerCase() const;
        String toUpperCase() const;
        String replace(char oldChar, char newChar) const;
        String replace(const StringView &oldStr, const StringView &newStr) const;

    protected:
        const char *_data;
        size_t _length;
    };

    // class StringViewSumHelper : public String
    // {
    // public:
    //     StringViewSumHelper(const StringView &sv) : String(sv.toString()) {}
    //     StringViewSumHelper(const String &s) : String(s) {}
    //     StringViewSumHelper(const char *p) : String(p) {}
    // };

    // inline StringViewSumHelper operator+(const StringView &lhs, const StringView &rhs)
    // {
    //     StringViewSumHelper result(lhs);
    //     result.concat(rhs.toString());
    //     return result;
    // }

    // inline StringViewSumHelper operator+(const StringView &lhs, const String &rhs)
    // {
    //     StringViewSumHelper result(lhs);
    //     result.concat(rhs);
    //     return result;
    // }

    // inline StringViewSumHelper operator+(const StringView &lhs, const char *rhs)
    // {
    //     StringViewSumHelper result(lhs);
    //     result.concat(rhs);
    //     return result;
    // }

    // inline StringViewSumHelper operator+(const String &lhs, const StringView &rhs)
    // {
    //     StringViewSumHelper result(lhs);
    //     result.concat(rhs.toString());
    //     return result;
    // }

    // inline StringViewSumHelper operator+(const char *lhs, const StringView &rhs)
    // {
    //     StringViewSumHelper result(lhs);
    //     result.concat(rhs.toString());
    //     return result;
    // }

    // inline StringViewSumHelper & operator+(StringViewSumHelper &lhs, const StringView &rhs)
    // {
    //     lhs.concat(rhs.toString());
    //     return lhs;
    // }

    // inline StringViewSumHelper & operator+(StringViewSumHelper &lhs, const String &rhs)
    // {
    //     lhs.concat(rhs);
    //     return lhs;
    // }

    // inline StringViewSumHelper & operator+(StringViewSumHelper &lhs, const char *rhs)
    // {
    //     lhs.concat(rhs);
    //     return lhs;
    // }
}