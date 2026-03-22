#pragma once
#include <Arduino.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Streams.h"
#include "Iterators.h"

namespace HttpServerAdvanced
{
    /**
     * @brief A stream that decodes URI-encoded data (percent-encoding and + for space).
     */
    class UriDecodingStream : public ReadStream
    {
    private:
        std::unique_ptr<ReadStream> innerStream_;
        enum class State
        {
            Normal,
            Percent1,
            Percent2
        } state_ = State::Normal;
        char percentBuffer_[2];
        size_t percentIndex_ = 0;

    public:
        UriDecodingStream(const String &uri);
        UriDecodingStream(const char *uri);
        UriDecodingStream(const uint8_t *uri, size_t length);
        UriDecodingStream(std::unique_ptr<ReadStream> innerStream);
        virtual int available() override;
        virtual int read() override;
        virtual int peek() override;
    };

    /**
     * @brief A stream that encodes data using URI percent-encoding.
     */
    class UriEncodingStream : public ReadStream
    {
    private:
        std::unique_ptr<ReadStream> innerStream_;
        enum class State
        {
            Normal,
            EncodedPercent,
            EncodedHex1,
            EncodedHex2
        } state_ = State::Normal;
        char encodedBuffer_[3]; // "%XX"
        size_t encodedIndex_ = 0;

    public:
        UriEncodingStream(const String &uri);
        UriEncodingStream(const char *uri);
        UriEncodingStream(const uint8_t *uri, size_t length);
        UriEncodingStream(std::unique_ptr<ReadStream> innerStream);
        virtual int available() override;
        virtual int read() override;
        virtual int peek() override;
    };

    class PairEncodingIterator : public BoundedStreamIterable<PairEncodingIterator>
    {
    private:
        static constexpr const char *ValueDelimiter = "=";
        std::pair<std::string_view, std::string_view> pair_;

        virtual bool compareValue(const PairEncodingIterator &other) const override
        {
            return pair_.first == other.pair_.first && pair_.second == other.pair_.second;
        }

    public:
        PairEncodingIterator(std::pair<std::string_view, std::string_view> pair, size_t index = 0, size_t maxIndex = 3)
            : BoundedStreamIterable<PairEncodingIterator>(index, maxIndex), pair_(pair) {}
        virtual value_type getAt(size_t index) const override
        {
            if (index == 0)
            {
                return std::make_unique<UriEncodingStream>(std::make_unique<OctetsStream>(pair_.first.data(), pair_.first.size()));
            }
            else if (index == 1)
            {
                return std::make_unique<UriEncodingStream>(std::make_unique<OctetsStream>(ValueDelimiter, 1));
            }
            else if (index == 2)
            {
                return std::make_unique<UriEncodingStream>(std::make_unique<OctetsStream>(pair_.second.data(), pair_.second.size()));
            }
            return nullptr;
        }
        static PairEncodingIterator begin(std::pair<std::string_view, std::string_view> pair)
        {
            size_t maxIndex = pair.second.empty() ? 1 : 3;
            return PairEncodingIterator(pair, 0, maxIndex);
        }
        static PairEncodingIterator end(std::pair<std::string_view, std::string_view> pair)
        {
            size_t maxIndex = pair.second.empty() ? 1 : 3;
            return PairEncodingIterator(pair, maxIndex, maxIndex);
        }
        static std::unique_ptr<ReadStream> createStream(std::pair<std::string_view, std::string_view> pair)
        {
            return std::make_unique<IndefiniteConcatStream<PairEncodingIterator>>(begin(pair), end(pair));
        }
    };

    class FormEncodingStream;

    class FormUrlEncodingIterator : public BoundedStreamIterable<FormUrlEncodingIterator>
    {
    private:
        static constexpr const char *PairDelimiter = "&";
        const std::vector<std::pair<std::string_view, std::string_view>> &pairs_;

        virtual bool compareValue(const FormUrlEncodingIterator &other) const override
        {
            return &pairs_ == &other.pairs_;
        }
        virtual value_type getAt(size_t index) const override
        {
            size_t pairIndex = index / 2;
            bool isValue = (index % 2) == 1;
            if (isValue)
            {
                if (pairIndex < pairs_.size() - 1)
                {
                    return std::make_unique<OctetsStream>(PairDelimiter, 1);
                }
                else
                {
                    // Should not happen
                    return nullptr;
                }
            }
            else
            {
                auto &pair = pairs_[pairIndex];
                return PairEncodingIterator::createStream(pair);
            }
        }

    public:
        FormUrlEncodingIterator(const std::vector<std::pair<std::string_view, std::string_view>> &pairs, size_t index = 0)
            : BoundedStreamIterable<FormUrlEncodingIterator>(index, pairs.size() * 2 - (pairs.empty() ? 0 : 1)), pairs_(pairs) {}

        static FormUrlEncodingIterator begin(const std::vector<std::pair<std::string_view, std::string_view>> &pairs)
        {
            return FormUrlEncodingIterator(pairs, 0);
        }
        static FormUrlEncodingIterator end(const std::vector<std::pair<std::string_view, std::string_view>> &pairs)
        {
            return FormUrlEncodingIterator(pairs, pairs.size() * 2 - (pairs.empty() ? 0 : 1));
        }
    };


    class FormEncodingStream : public IndefiniteConcatStream<FormUrlEncodingIterator, FormUrlEncodingIterator>
    {
        private:
         std::vector<std::pair<std::string, std::string>> data_;
         std::vector<std::pair<std::string_view,std::string_view>> viewData_;
         static std::vector<std::pair<std::string_view,std::string_view>> toViewData(const std::vector<std::pair<std::string, std::string>> & data)
         {
            std::vector<std::pair<std::string_view,std::string_view>> views;
            views.reserve(data.size());
            for (const auto & pair : data)
            {
                views.emplace_back(
                    std::string_view(pair.first.data(), pair.first.size()),
                    std::string_view(pair.second.data(), pair.second.size()));
            }
            return views;
         }
         static std::vector<std::pair<std::string, std::string>> toOwnedData(std::vector<std::pair<String, String>> && data)
         {
            std::vector<std::pair<std::string, std::string>> owned;
            owned.reserve(data.size());
            for (auto & pair : data)
            {
                owned.emplace_back(
                    std::string(pair.first.c_str(), pair.first.length()),
                    std::string(pair.second.c_str(), pair.second.length()));
            }
            return owned;
         }
         size_t totalLength_;
         size_t position_;
         static size_t calculateTotalLength(const std::vector<std::pair<std::string_view,std::string_view>> & viewData)
         {
            size_t total = 0;
            for (const auto & pair : viewData)
            {
                total += pair.first.size();
                if (!pair.second.empty())
                {
                    total += 1; // '='
                    total += pair.second.size();
                }
            }
            total += (viewData.size() > 1 ? (viewData.size() - 1) : 0); // '&' delimiters
            return total;
         }
        public:

        FormEncodingStream(std::vector<std::pair<std::string, std::string>> && data) : data_(std::move(data)), viewData_(toViewData(data_)),
        IndefiniteConcatStream<FormUrlEncodingIterator, FormUrlEncodingIterator>(FormUrlEncodingIterator::begin(viewData_), FormUrlEncodingIterator::end(viewData_)), 
        totalLength_(calculateTotalLength(viewData_)), position_(0) {}

        FormEncodingStream(std::vector<std::pair<String, String>> && data)
            : FormEncodingStream(toOwnedData(std::move(data))) {}

        virtual int available() override
        {
            return totalLength_ - position_;
        }

        virtual int read() override
        {
            int result = IndefiniteConcatStream<FormUrlEncodingIterator, FormUrlEncodingIterator>::read();
            if (result != -1)
            {
            ++position_;
            }
            return result;
        }
    };
}
