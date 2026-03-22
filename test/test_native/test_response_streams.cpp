#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/response/ChunkedHttpResponseBodyStream.h"
#include "../../src/response/HttpResponse.h"
#include "../../src/streams/ByteStream.h"
#include "../../src/streams/Streams.h"

#include <algorithm>
#include <cstring>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    String ReadByteSourceAsString(IByteSource &source)
    {
        String result;
        uint8_t buffer[16] = {};
        while (true)
        {
            const size_t bytesRead = source.read(HttpServerAdvanced::span<uint8_t>(buffer, sizeof(buffer)));
            if (bytesRead == 0)
            {
                break;
            }

            for (size_t index = 0; index < bytesRead; ++index)
            {
                result += static_cast<char>(buffer[index]);
            }
        }

        return result;
    }

    String DrainByteSourceWithAvailability(IByteSource &source, std::size_t maxUnavailablePolls = 8)
    {
        String result;
        uint8_t buffer[16] = {};
        std::size_t unavailablePolls = 0;

        while (true)
        {
            const AvailableResult available = source.available();
            if (available.hasBytes())
            {
                const size_t bytesRead = source.read(HttpServerAdvanced::span<uint8_t>(buffer, (std::min)(available.count, sizeof(buffer))));
                TEST_ASSERT_GREATER_THAN_UINT64(0, bytesRead);

                for (size_t index = 0; index < bytesRead; ++index)
                {
                    result += static_cast<char>(buffer[index]);
                }

                unavailablePolls = 0;
                continue;
            }

            if (available.isTemporarilyUnavailable())
            {
                ++unavailablePolls;
                TEST_ASSERT_LESS_OR_EQUAL_UINT64(maxUnavailablePolls, unavailablePolls);
                continue;
            }

            TEST_ASSERT_FALSE(available.hasError());
            break;
        }

        return result;
    }

    class VectorByteSource : public IByteSource
    {
    public:
        explicit VectorByteSource(const char *text, bool temporarilyUnavailableOnce = false)
            : bytes_(text, text + std::strlen(text)), temporarilyUnavailableOnce_(temporarilyUnavailableOnce)
        {
        }

        AvailableResult available() override
        {
            if (position_ < bytes_.size())
            {
                return AvailableBytes(bytes_.size() - position_);
            }

            if (temporarilyUnavailableOnce_ && !temporarilyUnavailableReported_)
            {
                temporarilyUnavailableReported_ = true;
                return TemporarilyUnavailableResult();
            }

            return ExhaustedResult();
        }

        size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            size_t totalRead = 0;
            while (totalRead < buffer.size() && position_ < bytes_.size())
            {
                buffer[totalRead++] = static_cast<uint8_t>(bytes_[position_++]);
            }

            return totalRead;
        }

        size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            size_t totalRead = 0;
            while (totalRead < buffer.size() && (position_ + totalRead) < bytes_.size())
            {
                buffer[totalRead] = static_cast<uint8_t>(bytes_[position_ + totalRead]);
                ++totalRead;
            }

            return totalRead;
        }

    private:
        std::vector<char> bytes_;
        std::size_t position_ = 0;
        bool temporarilyUnavailableOnce_ = false;
        bool temporarilyUnavailableReported_ = false;
    };

    class ScriptedByteSource : public IByteSource
    {
    public:
        explicit ScriptedByteSource(std::initializer_list<std::pair<const char *, bool>> steps)
        {
            for (const auto &step : steps)
            {
                steps_.push_back({step.first != nullptr ? step.first : "", step.second});
            }
        }

        AvailableResult available() override
        {
            advancePastConsumedSteps();
            if (stepIndex_ >= steps_.size())
            {
                return ExhaustedResult();
            }

            Step &step = steps_[stepIndex_];
            if (step.temporarilyUnavailable)
            {
                if (!step.unavailableReported)
                {
                    step.unavailableReported = true;
                    return TemporarilyUnavailableResult();
                }

                ++stepIndex_;
                return available();
            }

            return AvailableBytes(step.text.size() - positionInStep_);
        }

        size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            advancePastConsumedSteps();
            if (stepIndex_ >= steps_.size() || steps_[stepIndex_].temporarilyUnavailable)
            {
                return 0;
            }

            const std::string &text = steps_[stepIndex_].text;
            size_t totalRead = 0;
            while (totalRead < buffer.size() && positionInStep_ < text.size())
            {
                buffer[totalRead++] = static_cast<uint8_t>(text[positionInStep_++]);
            }

            advancePastConsumedSteps();
            return totalRead;
        }

        size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            advancePastConsumedSteps();
            if (stepIndex_ >= steps_.size() || steps_[stepIndex_].temporarilyUnavailable)
            {
                return 0;
            }

            const std::string &text = steps_[stepIndex_].text;
            size_t totalRead = 0;
            while (totalRead < buffer.size() && (positionInStep_ + totalRead) < text.size())
            {
                buffer[totalRead] = static_cast<uint8_t>(text[positionInStep_ + totalRead]);
                ++totalRead;
            }

            return totalRead;
        }

    private:
        struct Step
        {
            std::string text;
            bool temporarilyUnavailable = false;
            bool unavailableReported = false;
        };

        std::vector<Step> steps_;
        std::size_t stepIndex_ = 0;
        std::size_t positionInStep_ = 0;

        void advancePastConsumedSteps()
        {
            while (stepIndex_ < steps_.size())
            {
                Step &step = steps_[stepIndex_];
                if (step.temporarilyUnavailable)
                {
                    if (step.unavailableReported)
                    {
                        ++stepIndex_;
                        positionInStep_ = 0;
                        continue;
                    }

                    break;
                }

                if (positionInStep_ >= step.text.size())
                {
                    ++stepIndex_;
                    positionInStep_ = 0;
                    continue;
                }

                break;
            }
        }
    };

    HttpHeaderCollection MakeDeterministicResponseHeaders()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeader::Date("Thu, 01 Jan 1970 00:00:00 GMT"));
        headers.set(HttpHeader::Server("UnitTest"));
        headers.set(HttpHeader::ContentType("text/plain"));
        headers.set(HttpHeader::Connection("close"));
        return headers;
    }

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_byte_source_reads_direct_body_content()
    {
        auto body = std::make_unique<VectorByteSource>("ok");

        const String content = ReadByteSourceAsString(*body);
        TEST_ASSERT_TRUE(content == String("ok"));
    }

    void test_byte_source_reports_temporarily_unavailable()
    {
        auto body = std::make_unique<VectorByteSource>("", true);

        TEST_ASSERT_TRUE(body->available().isTemporarilyUnavailable());
    }

    void test_chunked_response_body_stream_reads_from_byte_source()
    {
        auto body = ChunkedHttpResponseBodyStream::create(std::make_unique<VectorByteSource>("Hi"));

        const String content = ReadByteSourceAsString(*body);
        TEST_ASSERT_TRUE(content == String("2\r\nHi\r\n0\r\n\r\n"));
    }

    void test_http_response_accepts_byte_source_body()
    {
        HttpHeaderCollection headers;
        HttpResponse response(HttpStatus::Ok(), std::make_unique<VectorByteSource>("body"), std::move(headers));

        auto body = response.getBody();
        const String content = ReadByteSourceAsString(*body);
        TEST_ASSERT_TRUE(content == String("body"));
    }

    void test_create_response_stream_serializes_direct_response_exactly()
    {
        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<VectorByteSource>("Hi"),
            MakeDeterministicResponseHeaders());

        auto source = CreateResponseStream(std::move(response));
        const String content = ReadByteSourceAsString(*source);

        TEST_ASSERT_TRUE(content == String(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "Hi"));
    }

    void test_create_response_stream_serializes_chunked_response_exactly()
    {
        auto response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"", true}, {"Hi", false}}),
            MakeDeterministicResponseHeaders());

        auto source = CreateResponseStream(std::move(response));
        const String content = DrainByteSourceWithAvailability(*source);

        TEST_ASSERT_TRUE(content == String(
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
            "Server: UnitTest\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "2\r\n"
            "Hi\r\n"
            "0\r\n\r\n"));
    }

    void test_chunked_response_stream_handles_temporary_unavailability_between_chunks()
    {
        auto body = ChunkedHttpResponseBodyStream::create(
            std::make_unique<ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"Hi", false}, {"", true}, {"!", false}}));

        const String content = DrainByteSourceWithAvailability(*body);
        TEST_ASSERT_TRUE(content == String("2\r\nHi\r\n1\r\n!\r\n0\r\n\r\n"));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_byte_source_reads_direct_body_content);
        RUN_TEST(test_byte_source_reports_temporarily_unavailable);
        RUN_TEST(test_chunked_response_body_stream_reads_from_byte_source);
        RUN_TEST(test_http_response_accepts_byte_source_body);
        RUN_TEST(test_create_response_stream_serializes_direct_response_exactly);
        RUN_TEST(test_create_response_stream_serializes_chunked_response_exactly);
        RUN_TEST(test_chunked_response_stream_handles_temporary_unavailability_between_chunks);
        return UNITY_END();
    }
}

int run_test_response_streams()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "response streams",
        runUnitySuite,
        localSetUp,
        localTearDown);
}