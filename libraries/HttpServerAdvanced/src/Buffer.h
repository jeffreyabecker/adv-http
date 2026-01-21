#pragma once

#include <memory>
#include <Arduino.h>

namespace HttpServerAdvanced
{
    class Buffer
    {
    private:
        const uint8_t* data_;
        size_t size_ = 0;
    public:
        Buffer(const uint8_t* data, size_t size) : data_(data), size_(size) {}
        const uint8_t* data() const { return data_; }
        size_t size() const { return size_; }

        const uint8_t* begin() const { return data_; }
        const uint8_t* end() const { return data_ + size_; }
    };
} // namespace HttpServerAdvanced