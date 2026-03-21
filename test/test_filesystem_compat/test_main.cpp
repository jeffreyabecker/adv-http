#include <unity.h>

#include <Arduino.h>

#include "../../src/compat/FileSystem.h"

#include <memory>
#include <string>
#include <utility>

using namespace HttpServerAdvanced;

void setUp()
{
}

void tearDown()
{
}

namespace
{
    class FakeFileImpl : public Compat::FileImpl
    {
    private:
        std::string name_;
        std::string data_;
        std::size_t readPos_ = 0;
        bool valid_ = true;
        bool directory_ = false;
        uint32_t lastWrite_ = 0;

    public:
        FakeFileImpl(std::string name, std::string data, uint32_t lastWrite = 0, bool directory = false)
            : name_(std::move(name)), data_(std::move(data)), directory_(directory), lastWrite_(lastWrite)
        {
        }

        bool isValid() const override
        {
            return valid_;
        }

        bool isDirectory() const override
        {
            return directory_;
        }

        void close() override
        {
            valid_ = false;
        }

        std::size_t size() const override
        {
            return data_.size();
        }

        const char *fullName() const override
        {
            return name_.c_str();
        }

        uint32_t getLastWrite() const override
        {
            return lastWrite_;
        }

        int available() override
        {
            return valid_ ? static_cast<int>(data_.size() - readPos_) : 0;
        }

        int read() override
        {
            if (!valid_ || readPos_ >= data_.size())
            {
                return -1;
            }
            return static_cast<uint8_t>(data_[readPos_++]);
        }

        int peek() override
        {
            if (!valid_ || readPos_ >= data_.size())
            {
                return -1;
            }
            return static_cast<uint8_t>(data_[readPos_]);
        }
    };

    class FakeFs : public FS
    {
    public:
        using FS::open;

        std::string lastPath;
        std::string lastMode;

        File open(const char *path, const char *mode) override
        {
            lastPath = path != nullptr ? path : "";
            lastMode = mode != nullptr ? mode : "";
            return File(std::make_shared<FakeFileImpl>(lastPath, "data", 1234));
        }
    };

    void test_default_file_is_invalid_and_safe()
    {
        File file;

        TEST_ASSERT_FALSE(static_cast<bool>(file));
        TEST_ASSERT_FALSE(file.isDirectory());
        TEST_ASSERT_EQUAL_INT(0, file.available());
        TEST_ASSERT_EQUAL_INT(-1, file.peek());
        TEST_ASSERT_EQUAL_INT(-1, file.read());
        TEST_ASSERT_EQUAL_UINT64(0, file.write('x'));
        TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(file.size()));
        TEST_ASSERT_EQUAL_STRING("", file.fullName());
        TEST_ASSERT_EQUAL_UINT32(0, file.getLastWrite());
    }

    void test_file_wraps_copyable_value_handle_and_has_noop_write_surface()
    {
        File file(std::make_shared<FakeFileImpl>("/www/index.html", "ab", 99));
        File copy = file;

        TEST_ASSERT_TRUE(static_cast<bool>(file));
        TEST_ASSERT_TRUE(static_cast<bool>(copy));
        TEST_ASSERT_EQUAL_STRING("/www/index.html", copy.fullName());
        TEST_ASSERT_EQUAL_UINT32(99, copy.getLastWrite());
        TEST_ASSERT_EQUAL_UINT64(0, copy.write("cd", 2));
        TEST_ASSERT_EQUAL_UINT64(0, copy.write(reinterpret_cast<const uint8_t *>("ef"), 2));
        TEST_ASSERT_EQUAL_INT(0, copy.availableForWrite());
        copy.flush();
        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(file.size()));
    }

    void test_fs_open_returns_file_by_value_and_accepts_string_like_paths()
    {
        FakeFs fs;
        FS &filesystem = fs;
        String path("/www/app.js");

        File file = filesystem.open(path, "r");

        TEST_ASSERT_TRUE(static_cast<bool>(file));
        TEST_ASSERT_EQUAL_STRING("/www/app.js", fs.lastPath.c_str());
        TEST_ASSERT_EQUAL_STRING("r", fs.lastMode.c_str());
        TEST_ASSERT_EQUAL_STRING("/www/app.js", file.fullName());
        TEST_ASSERT_EQUAL_INT('d', file.read());
    }
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_default_file_is_invalid_and_safe);
    RUN_TEST(test_file_wraps_copyable_value_handle_and_has_noop_write_surface);
    RUN_TEST(test_fs_open_returns_file_by_value_and_accepts_string_like_paths);
    return UNITY_END();
}