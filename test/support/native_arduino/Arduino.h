#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <string>
#include <type_traits>
#include <utility>

using byte = uint8_t;

class String
{
public:
    String() = default;
    String(const char *value) : value_(value != nullptr ? value : "") {}
    String(const std::string &value) : value_(value) {}
    String(std::string &&value) noexcept : value_(std::move(value)) {}
    String(char value) : value_(1, value) {}

    template <typename Integer, typename = std::enable_if_t<std::is_integral_v<Integer>>>
    String(Integer value) : value_(std::to_string(value)) {}

    String(const String &) = default;
    String(String &&) noexcept = default;
    String &operator=(const String &) = default;
    String &operator=(String &&) noexcept = default;

    const char *c_str() const
    {
        return value_.c_str();
    }

    std::size_t length() const
    {
        return value_.size();
    }

    bool isEmpty() const
    {
        return value_.empty();
    }

    void reserve(std::size_t size)
    {
        value_.reserve(size);
    }

    String &operator+=(char value)
    {
        value_ += value;
        return *this;
    }

    String &operator+=(const char *value)
    {
        value_ += (value != nullptr ? value : "");
        return *this;
    }

    friend bool operator==(const String &left, const String &right)
    {
        return left.value_ == right.value_;
    }

    friend bool operator!=(const String &left, const String &right)
    {
        return !(left == right);
    }

private:
    std::string value_;
};
