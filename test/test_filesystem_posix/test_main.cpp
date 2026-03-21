#include "../../test/support/native_unity/unity.h"
#include "../../src/compat/PosixFileAdapter.h"

using namespace HttpServerAdvanced;

void test_posix_adapter_can_open_and_read_temp_file()
{
    const char* tmpPath = "test_temp_file.txt";
    FILE* f = fopen(tmpPath, "wb");
    if (!f) {
        TEST_ASSERT_TRUE(false);
        return;
    }
    const char* content = "hello-posix";
    fwrite(content, 1, strlen(content), f);
    fclose(f);

    Compat::PosixFS fs;
    File file = fs.open(tmpPath, "rb");
    TEST_ASSERT_TRUE(static_cast<bool>(file));
    TEST_ASSERT_EQUAL_UINT64(strlen(content), file.size());
    TEST_ASSERT_EQUAL_INT('h', file.read());
    // cleanup
    remove(tmpPath);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_posix_adapter_can_open_and_read_temp_file);
    return UNITY_END();
}
