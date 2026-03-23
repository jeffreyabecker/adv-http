#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/compat/PosixFileAdapter.h"

#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_posix_adapter_can_open_and_read_temp_file()
    {
        const char *tmpPath = "test_temp_file.txt";
        FILE *fileHandle = fopen(tmpPath, "wb");
        if (fileHandle == nullptr)
        {
            TEST_ASSERT_TRUE(false);
            return;
        }

        const char *content = "hello-posix";
        fwrite(content, 1, strlen(content), fileHandle);
        fclose(fileHandle);

        Compat::PosixFS fs;
        FileHandle file = fs.open(tmpPath, FileOpenMode::Read);
        TEST_ASSERT_NOT_NULL(file.get());
        TEST_ASSERT_TRUE(file->size().has_value());
        TEST_ASSERT_EQUAL_UINT64(strlen(content), *file->size());
        TEST_ASSERT_EQUAL_INT('h', ReadByte(*file));

        file->close();
        TEST_ASSERT_EQUAL_INT(0, remove(tmpPath));
    }

    void test_posix_adapter_supports_directory_handles()
    {
        Compat::PosixFS fs;

        FileHandle file = fs.open(".", FileOpenMode::Read);

        TEST_ASSERT_NOT_NULL(file.get());
        TEST_ASSERT_TRUE(file->isDirectory());
        TEST_ASSERT_FALSE(file->size().has_value());
        TEST_ASSERT_EQUAL_UINT64(0, file->read(HttpServerAdvanced::span<uint8_t>()));
    }

    void test_posix_adapter_can_write_and_reopen_file()
    {
        const char *tmpPath = "test_temp_file_write.txt";
        remove(tmpPath);

        Compat::PosixFS fs;
        FileHandle writable = fs.open(tmpPath, FileOpenMode::ReadWrite);
        TEST_ASSERT_NOT_NULL(writable.get());

        const std::vector<uint8_t> payload = {'o', 'k'};
        TEST_ASSERT_EQUAL_UINT64(payload.size(), writable->write(HttpServerAdvanced::span<const uint8_t>(payload.data(), payload.size())));
        writable->flush();
        writable->close();

        FileHandle readable = fs.open(tmpPath, FileOpenMode::Read);
        TEST_ASSERT_NOT_NULL(readable.get());
        TEST_ASSERT_TRUE(readable->size().has_value());
        TEST_ASSERT_EQUAL_UINT64(payload.size(), *readable->size());
        TEST_ASSERT_EQUAL_INT('o', ReadByte(*readable));
        TEST_ASSERT_EQUAL_INT('k', ReadByte(*readable));
        readable->close();

        TEST_ASSERT_EQUAL_INT(0, remove(tmpPath));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_posix_adapter_can_open_and_read_temp_file);
        RUN_TEST(test_posix_adapter_supports_directory_handles);
        RUN_TEST(test_posix_adapter_can_write_and_reopen_file);
        return UNITY_END();
    }
}

int run_test_filesystem_posix()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "filesystem posix",
        runUnitySuite,
        localSetUp,
        localTearDown);
}