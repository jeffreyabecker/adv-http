#include <unity.h>

#include "../../src/streams/Streams.h"
#include "../../src/streams/Base64Stream.h"

#include "../../src/streams/Streams.cpp"
#include "../../src/streams/Base64Stream.cpp"

#include <memory>
#include <vector>

using namespace HttpServerAdvanced;

void setUp()
{
}

void tearDown()
{
}

namespace
{
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
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_read_stream_reports_end_of_stream);
    RUN_TEST(test_octets_stream_reads_and_peeks_without_consuming_peek);
    RUN_TEST(test_lazy_stream_adapter_creates_stream_once);
    RUN_TEST(test_non_owning_memory_stream_supports_read_write_and_compaction);
    RUN_TEST(test_non_owning_memory_stream_exposes_bulk_write_overloads);
    RUN_TEST(test_buffered_read_stream_wrapper_buffers_peek_and_read);
    RUN_TEST(test_concat_stream_reads_all_children_in_order);
    RUN_TEST(test_read_helpers_convert_stream_contents);
    RUN_TEST(test_base64_encoder_stream_encodes_expected_output);
    RUN_TEST(test_base64_decoder_stream_decodes_expected_output);
    return UNITY_END();
}