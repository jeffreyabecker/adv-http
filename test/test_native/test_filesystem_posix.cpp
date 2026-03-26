#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/compat/PosixFileAdapter.h"
#include "../../src/core/HttpContentTypes.h"
#include "../../src/staticfiles/DefaultFileLocator.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string_view>
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
        std::ofstream fileHandle(tmpPath, std::ios::binary | std::ios::trunc);
        if (!fileHandle.is_open())
        {
            TEST_ASSERT_TRUE(false);
            return;
        }

        const char *content = "hello-posix";
        fileHandle.write(content, static_cast<std::streamsize>(std::strlen(content)));
        fileHandle.close();

        Compat::PosixFS fs;
        FileHandle file = fs.open(tmpPath, FileOpenMode::Read);
        TEST_ASSERT_NOT_NULL(file.get());
        TEST_ASSERT_TRUE(file->size().has_value());
        TEST_ASSERT_EQUAL_UINT64(std::strlen(content), *file->size());
        TEST_ASSERT_EQUAL_INT('h', ReadByte(*file));

        file->close();
        TEST_ASSERT_EQUAL_INT(0, std::remove(tmpPath));
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
        std::remove(tmpPath);

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

        TEST_ASSERT_EQUAL_INT(0, std::remove(tmpPath));
    }

    void test_content_types_accept_string_view_inputs()
    {
        HttpContentTypes contentTypes;

        TEST_ASSERT_EQUAL_STRING("text/html", contentTypes.getContentTypeByExtension(std::string_view("HTML")));
        TEST_ASSERT_EQUAL_STRING("application/javascript", contentTypes.getContentTypeFromPath(std::string_view("/www/app.Js")));

        contentTypes.set(std::string_view("cust"), "application/x-custom");
        TEST_ASSERT_EQUAL_STRING("application/x-custom", contentTypes.getContentTypeByExtension(std::string_view("CUST")));
    }

    void test_default_file_locator_accepts_string_view_configuration()
    {
        Compat::PosixFS fs;
        DefaultFileLocator locator(fs);

        locator.setRequestPathPrefixes(std::string_view("/static"), std::string_view("/static/api"));

        TEST_ASSERT_TRUE(locator.canHandle(std::string_view("/static/app.js")));
        TEST_ASSERT_FALSE(locator.canHandle(std::string_view("/static/api/data.json")));

        locator.setRequestPathPrefixes(std::string_view("/assets"));
        TEST_ASSERT_TRUE(locator.canHandle(std::string_view("/assets/logo.svg")));
        TEST_ASSERT_FALSE(locator.canHandle(std::string_view("/static/app.js")));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_posix_adapter_can_open_and_read_temp_file);
        RUN_TEST(test_posix_adapter_supports_directory_handles);
        RUN_TEST(test_posix_adapter_can_write_and_reopen_file);
        RUN_TEST(test_content_types_accept_string_view_inputs);
        RUN_TEST(test_default_file_locator_accepts_string_view_configuration);
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