#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/streams/Streams.h"
#include "../../src/streams/Base64Stream.h"

#include <cstring>
#include <memory>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    class NegativeAvailableReadStream : public ReadStream
    {
    public:
        explicit NegativeAvailableReadStream(const char *text)
        {
            const auto *begin = reinterpret_cast<const uint8_t *>(text);
            bytes_.assign(begin, begin + std::strlen(text));
        }

        NegativeAvailableReadStream(const uint8_t *data, std::size_t length)
            : bytes_(data, data + length)
        {
        }

        int available() override
        {
            return position_ < bytes_.size() ? -1 : 0;
        }

        int read() override
        {
            if (position_ >= bytes_.size())
            {
                return -1;
            }

            return bytes_[position_++];
        }

        int peek() override
        {
            if (position_ >= bytes_.size())
            {
                return -1;
            }

            return bytes_[position_];
        }

    private:
        std::vector<uint8_t> bytes_;
        std::size_t position_ = 0;
    };

    class TemporarilyUnavailableReadStream : public ReadStream
    {
    public:
        int available() override
        {
            return -1;
        }

        int read() override
        {
            return -1;
        }

        int peek() override
        {
            return -1;
        }
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_empty_read_stream_reports_end_of_stream()
    {
        EmptyReadStream stream;

        TEST_ASSERT_EQUAL_INT(0, stream.available());
        TEST_ASSERT_EQUAL_INT(-1, stream.peek());
        TEST_ASSERT_EQUAL_INT(-1, stream.read());
    }

    void test_octets_stream_reads_and_peeks_without_consuming_peek()
    {
        OctetsStream stream("abc");

        TEST_ASSERT_EQUAL_INT(3, stream.available());
        TEST_ASSERT_EQUAL_INT('a', stream.peek());
        TEST_ASSERT_EQUAL_INT(3, stream.available());
        TEST_ASSERT_EQUAL_INT('a', stream.read());
        TEST_ASSERT_EQUAL_INT(2, stream.available());
        TEST_ASSERT_EQUAL_INT('b', stream.read());
        TEST_ASSERT_EQUAL_INT('c', stream.read());
        TEST_ASSERT_EQUAL_INT(0, stream.available());
        TEST_ASSERT_EQUAL_INT(-1, stream.read());
    }

    void test_lazy_stream_adapter_creates_stream_once()
    {
        int factoryCallCount = 0;
        LazyStreamAdapter stream([&factoryCallCount]() {
            ++factoryCallCount;
            return std::make_unique<OctetsStream>("xy");
        });

        TEST_ASSERT_EQUAL_INT(0, factoryCallCount);
        TEST_ASSERT_EQUAL_INT(2, stream.available());
        TEST_ASSERT_EQUAL_INT(1, factoryCallCount);
        TEST_ASSERT_EQUAL_INT('x', stream.peek());
        TEST_ASSERT_EQUAL_INT(1, factoryCallCount);
        TEST_ASSERT_EQUAL_INT('x', stream.read());
        TEST_ASSERT_EQUAL_INT('y', stream.read());
        TEST_ASSERT_EQUAL_INT(1, factoryCallCount);
    }

    void test_non_owning_memory_stream_supports_read_write_and_compaction()
    {
        uint8_t buffer[4] = {};
        NonOwningMemoryStream stream(buffer, sizeof(buffer));

        TEST_ASSERT_EQUAL_INT(4, stream.availableForWrite());
        TEST_ASSERT_EQUAL_UINT64(1, stream.write('a'));
        TEST_ASSERT_EQUAL_UINT64(1, stream.write('b'));
        TEST_ASSERT_EQUAL_UINT64(1, stream.write('c'));
        TEST_ASSERT_EQUAL_INT(3, stream.available());
        TEST_ASSERT_EQUAL_INT('a', stream.read());
        TEST_ASSERT_EQUAL_INT('b', stream.read());
        TEST_ASSERT_EQUAL_INT(3, stream.availableForWrite());
        TEST_ASSERT_EQUAL_UINT64(1, stream.write('d'));
        TEST_ASSERT_EQUAL_UINT64(1, stream.write('e'));
        TEST_ASSERT_EQUAL_INT('c', stream.read());
        TEST_ASSERT_EQUAL_INT('d', stream.read());
        TEST_ASSERT_EQUAL_INT('e', stream.read());
        TEST_ASSERT_EQUAL_INT(-1, stream.read());
    }

    void test_non_owning_memory_stream_exposes_bulk_write_overloads()
    {
        uint8_t buffer[8] = {};
        NonOwningMemoryStream stream(buffer, sizeof(buffer));
        const uint8_t suffix[] = {'c', 'd'};

        TEST_ASSERT_EQUAL_UINT64(2, stream.write("ab", 2));
        TEST_ASSERT_EQUAL_UINT64(2, stream.write(suffix, sizeof(suffix)));
        TEST_ASSERT_EQUAL_INT(4, stream.available());
        TEST_ASSERT_EQUAL_INT('a', stream.read());
        TEST_ASSERT_EQUAL_INT('b', stream.read());
        TEST_ASSERT_EQUAL_INT('c', stream.read());
        TEST_ASSERT_EQUAL_INT('d', stream.read());
    }

    void test_buffered_read_stream_wrapper_buffers_peek_and_read()
    {
        auto inner = std::make_unique<OctetsStream>("hello");
        StaticBufferedReadStreamWrapper<3> stream(std::move(inner));

        TEST_ASSERT_EQUAL_INT('h', stream.peek());
        TEST_ASSERT_EQUAL_INT('h', stream.read());
        TEST_ASSERT_EQUAL_INT('e', stream.peek());
        TEST_ASSERT_EQUAL_INT('e', stream.read());
        TEST_ASSERT_EQUAL_INT('l', stream.read());
        TEST_ASSERT_EQUAL_INT('l', stream.read());
        TEST_ASSERT_EQUAL_INT('o', stream.read());
        TEST_ASSERT_EQUAL_INT(-1, stream.peek());
    }

    void test_buffered_read_stream_wrapper_available_reports_only_buffered_bytes()
    {
        auto inner = std::make_unique<OctetsStream>("hi");
        StaticBufferedReadStreamWrapper<3> stream(std::move(inner));

        TEST_ASSERT_EQUAL_INT(0, stream.available());
        TEST_ASSERT_EQUAL_INT('h', stream.peek());
        TEST_ASSERT_EQUAL_INT(2, stream.available());
    }

    void test_concat_stream_reads_all_children_in_order()
    {
        std::array<std::unique_ptr<Stream>, 3> streams = {
            std::make_unique<OctetsStream>("ab"),
            std::make_unique<EmptyReadStream>(),
            std::make_unique<OctetsStream>("cd")};
        ConcatStream<3> stream(std::move(streams));

        TEST_ASSERT_EQUAL_INT('a', stream.read());
        TEST_ASSERT_EQUAL_INT('b', stream.read());
        TEST_ASSERT_EQUAL_INT('c', stream.read());
        TEST_ASSERT_EQUAL_INT('d', stream.read());
        TEST_ASSERT_EQUAL_INT(-1, stream.read());
    }

    void test_concat_stream_available_skips_only_exhausted_children()
    {
        std::array<std::unique_ptr<Stream>, 3> streams = {
            std::make_unique<EmptyReadStream>(),
            std::make_unique<OctetsStream>("ab"),
            std::make_unique<OctetsStream>("cd")};
        ConcatStream<3> stream(std::move(streams));

        TEST_ASSERT_EQUAL_INT(2, stream.available());
    }

    void test_concat_stream_peek_and_read_advance_past_negative_available_child()
    {
        std::array<std::unique_ptr<Stream>, 2> streams = {
            std::make_unique<TemporarilyUnavailableReadStream>(),
            std::make_unique<OctetsStream>("z")};
        ConcatStream<2> stream(std::move(streams));

        TEST_ASSERT_EQUAL_INT(-1, stream.available());
        TEST_ASSERT_EQUAL_INT('z', stream.peek());
        TEST_ASSERT_EQUAL_INT('z', stream.read());
        TEST_ASSERT_EQUAL_INT(0, stream.available());
    }

    void test_read_helpers_convert_stream_contents()
    {
        OctetsStream stringStream("stream");
        String result = ReadAsString(stringStream);
        TEST_ASSERT_TRUE(result == String("stream"));

        uint8_t octets[] = {1, 2, 3, 4};
        OctetsStream vectorStream(octets, sizeof(octets));
        std::vector<uint8_t> vector = ReadAsVector(vectorStream);
        TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(vector.size()));
        TEST_ASSERT_EQUAL_UINT8(1, vector[0]);
        TEST_ASSERT_EQUAL_UINT8(4, vector[3]);
    }

    void test_read_as_string_reads_when_available_is_negative()
    {
        NegativeAvailableReadStream stream("queued");

        String result = ReadAsString(stream);

        TEST_ASSERT_TRUE(result == String("queued"));
    }

    void test_read_as_vector_reads_when_available_is_negative()
    {
        const uint8_t octets[] = {5, 6, 7};
        NegativeAvailableReadStream stream(octets, sizeof(octets));

        std::vector<uint8_t> result = ReadAsVector(stream);

        TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(result.size()));
        TEST_ASSERT_EQUAL_UINT8(5, result[0]);
        TEST_ASSERT_EQUAL_UINT8(7, result[2]);
    }

    void test_base64_encoder_stream_encodes_expected_output()
    {
        auto encoder = Base64EncoderStream::create("Man");
        String result = ReadAsString(encoder);

        TEST_ASSERT_TRUE(result == String("TWFu"));
    }

    void test_base64_decoder_stream_decodes_expected_output()
    {
        auto decoder = Base64DecoderStream::create("TWFu");
        String result = ReadAsString(decoder);

        TEST_ASSERT_TRUE(result == String("Man"));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_empty_read_stream_reports_end_of_stream);
        RUN_TEST(test_octets_stream_reads_and_peeks_without_consuming_peek);
        RUN_TEST(test_lazy_stream_adapter_creates_stream_once);
        RUN_TEST(test_non_owning_memory_stream_supports_read_write_and_compaction);
        RUN_TEST(test_non_owning_memory_stream_exposes_bulk_write_overloads);
        RUN_TEST(test_buffered_read_stream_wrapper_buffers_peek_and_read);
        RUN_TEST(test_buffered_read_stream_wrapper_available_reports_only_buffered_bytes);
        RUN_TEST(test_concat_stream_reads_all_children_in_order);
        RUN_TEST(test_concat_stream_available_skips_only_exhausted_children);
        RUN_TEST(test_concat_stream_peek_and_read_advance_past_negative_available_child);
        RUN_TEST(test_read_helpers_convert_stream_contents);
        RUN_TEST(test_read_as_string_reads_when_available_is_negative);
        RUN_TEST(test_read_as_vector_reads_when_available_is_negative);
        RUN_TEST(test_base64_encoder_stream_encodes_expected_output);
        RUN_TEST(test_base64_decoder_stream_decodes_expected_output);
        return UNITY_END();
    }
}

int run_test_stream_utilities()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "stream utilities",
        runUnitySuite,
        localSetUp,
        localTearDown);
}