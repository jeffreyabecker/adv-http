#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/compat/PosixFileAdapter.h"

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
        File file = fs.open(tmpPath, "rb");
        TEST_ASSERT_TRUE(static_cast<bool>(file));
        TEST_ASSERT_EQUAL_UINT64(strlen(content), file.size());
        TEST_ASSERT_EQUAL_INT('h', file.read());

        file.close();
        TEST_ASSERT_EQUAL_INT(0, remove(tmpPath));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_posix_adapter_can_open_and_read_temp_file);
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