#pragma once
#include <cstddef>
#include "./Streams.h"  


namespace HttpServerAdvanced
{
    template <typename TSelf>
    class BoundedStreamIterable
    {
    public:
        using value_type = std::unique_ptr<ReadStream>;
        using reference = std::unique_ptr<ReadStream> &;
        using pointer = value_type *;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

    private:
        size_t index_ = 0;
        size_t maxIndex_;

        mutable std::unique_ptr<ReadStream> current_ = nullptr;

    protected:
        BoundedStreamIterable(size_t index, size_t maxIndex)
            : index_(index), maxIndex_(maxIndex) {}
        virtual value_type getAt(size_t index) const = 0;

    public:
        // Copyable to satisfy forward iterator requirements; reset transient current_
        BoundedStreamIterable(const BoundedStreamIterable &other)
            : index_(other.index_), maxIndex_(other.maxIndex_), current_(nullptr) {}
        BoundedStreamIterable &operator=(const BoundedStreamIterable &other)
        {
            if (this != &other)
            {
                index_ = other.index_;
                maxIndex_ = other.maxIndex_;
                current_.reset();
            }
            return *this;
        }

        reference operator*() const
        {
            if (current_ == nullptr && index_ < maxIndex_)
            {
                current_ = getAt(index_);
            }
            return current_;
        }

        TSelf &operator++()
        {
            ++index_;
            current_ = nullptr;
            return static_cast<TSelf &>(*this);
        }

        bool operator==(const TSelf &other) const
        {
            return index_ == other.index_;
        }

        bool operator!=(const TSelf &other) const
        {
            return index_ != other.index_;
        }

    };

    template <typename TSelf, size_t N>
    class FixedStreamIterable : public BoundedStreamIterable<TSelf>
    {
    public:
        FixedStreamIterable(size_t index)
            : BoundedStreamIterable<TSelf>(index, N) {}
        template <typename... Args>
        static TSelf begin(Args &&...args)
        {
            return TSelf(0, std::forward<Args>(args)...);
        }
        template <typename... Args>
        static TSelf end(Args &&...args)
        {
            return TSelf(N, std::forward<Args>(args)...);
        }
    };

}