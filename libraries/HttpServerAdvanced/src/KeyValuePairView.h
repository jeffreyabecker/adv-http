#pragma once
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <optional>
#include "./HttpUtility.h"
namespace HttpServerAdvanced
{
    template <typename TKey, typename TValue>
    class KeyValuePairView
    {
    protected:
        std::vector<std::pair<TKey, TValue>> pairs_;

    public:
        KeyValuePairView() : pairs_() {}
        KeyValuePairView(const std::vector<std::pair<TKey, TValue>> &pairs)
            : pairs_(pairs) {}
        KeyValuePairView(const KeyValuePairView &that) : pairs_(that.pairs_) {}
        KeyValuePairView(KeyValuePairView &&that) : pairs_(std::move(that.pairs_)) {}
        KeyValuePairView &operator=(const KeyValuePairView &that)
        {
            pairs_ = that.pairs_;
            return *this;
        }
        KeyValuePairView &operator=(KeyValuePairView &&that)
        {
            pairs_ = std::move(that.pairs_);
            return *this;
        }
        virtual std::size_t exists(const TKey &key) const
        {
            std::size_t cnt = 0;
            for (const auto &kv : pairs_)
            {
                if (kv.first == key)
                    ++cnt;
            }
            return cnt;
        }
        // Gets the value for a key, or returns std::nullopt if not found
        std::optional<TValue> get(const TKey &key) const
        {
            for (const auto &kv : pairs_)
            {
                if (kv.first == key)
                    return kv.second;
            }
            return std::nullopt;
        }
        std::vector<TValue> getAll(const TKey &key) const
        {
            std::vector<TValue> values;
            for (const auto &kv : pairs_)
            {
                if (kv.first == key)
                    values.push_back(kv.second);
            }
            return values;
        }

        const std::vector<std::pair<TKey, TValue>> &pairs() const
        {
            return pairs_;
        }
    };
}