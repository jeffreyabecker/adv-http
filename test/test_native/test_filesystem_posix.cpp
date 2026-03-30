#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#if defined(_WIN32)
#include "../../src/compat/platform/windows/WindowsFileAdapter.h"
#else
#include "../../src/compat/platform/posix/PosixFileAdapter.h"
#endif

#if defined(_WIN32)
using NativeFSImpl = HttpServerAdvanced::platform::windows::WindowsFs;
#else
using NativeFSImpl = HttpServerAdvanced::platform::posix::PosixFS;
#endif
#include "../../src/core/HttpContentTypes.h"
#include "../../src/staticfiles/DefaultFileLocator.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    struct OwnedDirectoryEntry
    {
        std::string name;
        std::string path;
        bool isDirectory = false;
    };

    std::string MakeTempPath(const char *label)
    {
        static unsigned counter = 0;
        return std::string("test_temp_") + label + "_" +
               std::to_string(static_cast<unsigned long long>(std::time(nullptr))) + "_" +
               std::to_string(counter++);
    }

    std::string JoinHostPath(std::string_view left, std::string_view right)
    {
#if defined(_WIN32)
        static constexpr char Separator = '\\';
#else
        static constexpr char Separator = '/';
#endif

        std::string joined(left);
        if (!joined.empty() && joined.back() != Separator)
        {
            joined.push_back(Separator);
        }

        joined.append(right.data(), right.size());
        return joined;
    }

    bool CreateBinaryFile(std::string_view hostPath, std::string_view content)
    {
        std::ofstream fileHandle(std::string(hostPath), std::ios::binary | std::ios::trunc);
        if (!fileHandle.is_open())
        {
            return false;
        }

        fileHandle.write(content.data(), static_cast<std::streamsize>(content.size()));
        return fileHandle.good();
    }

    void RemoveTree(std::string_view rootPath)
    {
        std::unique_ptr<IFileSystem> fs = std::make_unique<NativeFSImpl>();
        if (!fs)
        {
            return;
        }

        std::vector<OwnedDirectoryEntry> entries;
        if (fs->list(
                rootPath,
                [&entries](const DirectoryEntry &entry)
                {
                    entries.push_back(OwnedDirectoryEntry{std::string(entry.name), std::string(entry.path), entry.isDirectory});
                    return true;
                },
                true))
        {
            std::sort(entries.begin(), entries.end(),
                      [](const OwnedDirectoryEntry &left, const OwnedDirectoryEntry &right)
                      {
                          if (left.isDirectory != right.isDirectory)
                          {
                              return !left.isDirectory && right.isDirectory;
                          }

                          return left.path.size() > right.path.size();
                      });

            for (const OwnedDirectoryEntry &entry : entries)
            {
                fs->remove(entry.path);
            }
        }

        fs->remove(rootPath);
    }

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_native_file_factory_can_open_and_read_temp_file()
    {
        const std::string tmpPath = MakeTempPath("file_read") + ".txt";
        const char *content = "hello-posix";
        TEST_ASSERT_TRUE(CreateBinaryFile(tmpPath, content));

        std::unique_ptr<IFileSystem> fs = std::make_unique<NativeFSImpl>();
        TEST_ASSERT_NOT_NULL(fs.get());

        FileHandle file = fs->open(tmpPath, FileOpenMode::Read);
        TEST_ASSERT_NOT_NULL(file.get());
        TEST_ASSERT_TRUE(file->size().has_value());
        TEST_ASSERT_EQUAL_UINT64(std::strlen(content), *file->size());
        TEST_ASSERT_EQUAL_INT('h', ReadByte(*file));

        file->close();
        fs->remove(tmpPath);
    }

    void test_native_file_factory_supports_directory_handles()
    {
        std::unique_ptr<IFileSystem> fs = std::make_unique<NativeFSImpl>();
        TEST_ASSERT_NOT_NULL(fs.get());

        FileHandle file = fs->open(".", FileOpenMode::Read);

        TEST_ASSERT_NOT_NULL(file.get());
        TEST_ASSERT_TRUE(file->isDirectory());
        TEST_ASSERT_FALSE(file->size().has_value());
        TEST_ASSERT_EQUAL_UINT64(0, file->read(HttpServerAdvanced::span<uint8_t>()));
    }

    void test_native_file_factory_can_write_and_reopen_file()
    {
        const std::string tmpPath = MakeTempPath("file_write") + ".txt";

        std::unique_ptr<IFileSystem> fs = std::make_unique<NativeFSImpl>();
        TEST_ASSERT_NOT_NULL(fs.get());

        FileHandle writable = fs->open(tmpPath, FileOpenMode::ReadWrite);
        TEST_ASSERT_NOT_NULL(writable.get());

        const std::vector<uint8_t> payload = {'o', 'k'};
        TEST_ASSERT_EQUAL_UINT64(payload.size(),
                     writable->write(HttpServerAdvanced::span<const uint8_t>(payload.data(), payload.size())));
        writable->flush();
        writable->close();

        FileHandle readable = fs->open(tmpPath, FileOpenMode::Read);
        TEST_ASSERT_NOT_NULL(readable.get());
        TEST_ASSERT_TRUE(readable->size().has_value());
        TEST_ASSERT_EQUAL_UINT64(payload.size(), *readable->size());
        TEST_ASSERT_EQUAL_INT('o', ReadByte(*readable));
        TEST_ASSERT_EQUAL_INT('k', ReadByte(*readable));
        readable->close();

        fs->remove(tmpPath);
    }

    void test_native_file_factory_chroots_access_and_exposes_virtual_paths()
    {
        const std::string rootPath = MakeTempPath("chroot_root");
        const std::string nestedPath = JoinHostPath(rootPath, "nested");
        const std::string insidePath = JoinHostPath(nestedPath, "asset.txt");
        const std::string outsidePath = MakeTempPath("outside") + ".txt";

        std::unique_ptr<IFileSystem> hostFs = std::make_unique<NativeFSImpl>();
        TEST_ASSERT_NOT_NULL(hostFs.get());
        TEST_ASSERT_TRUE(hostFs->mkdir(rootPath));
        TEST_ASSERT_TRUE(hostFs->mkdir(nestedPath));
        TEST_ASSERT_TRUE(CreateBinaryFile(insidePath, "inside"));
        TEST_ASSERT_TRUE(CreateBinaryFile(outsidePath, "outside"));

        std::unique_ptr<IFileSystem> rootedFs = std::make_unique<NativeFSImpl>(rootPath);
        TEST_ASSERT_NOT_NULL(rootedFs.get());

        TEST_ASSERT_TRUE(rootedFs->exists("/nested/asset.txt"));
        TEST_ASSERT_TRUE(rootedFs->exists("nested\\asset.txt"));
        TEST_ASSERT_FALSE(rootedFs->exists("../outside.txt"));
        TEST_ASSERT_NULL(rootedFs->open("../outside.txt", FileOpenMode::Read).get());

        std::vector<OwnedDirectoryEntry> entries;
        TEST_ASSERT_TRUE(rootedFs->list(
            "/",
            [&entries](const DirectoryEntry &entry)
            {
                entries.push_back(OwnedDirectoryEntry{std::string(entry.name), std::string(entry.path), entry.isDirectory});
                return true;
            },
            true));

        bool foundDirectory = false;
        bool foundFile = false;
        for (const OwnedDirectoryEntry &entry : entries)
        {
            if (entry.isDirectory && entry.path == "/nested")
            {
                foundDirectory = true;
            }

            if (!entry.isDirectory && entry.path == "/nested/asset.txt")
            {
                foundFile = true;
            }
        }

        TEST_ASSERT_TRUE(foundDirectory);
        TEST_ASSERT_TRUE(foundFile);

        hostFs->remove(outsidePath);
        RemoveTree(rootPath);
    }

    void test_native_file_factory_normalizes_host_and_virtual_path_mangling()
    {
        const std::string rootPath = MakeTempPath("mangling_root");
        const std::string nestedPath = JoinHostPath(rootPath, "nested");
        const std::string filePath = JoinHostPath(nestedPath, "asset.txt");

        std::unique_ptr<IFileSystem> hostFs = std::make_unique<NativeFSImpl>();
        TEST_ASSERT_NOT_NULL(hostFs.get());
        TEST_ASSERT_TRUE(hostFs->mkdir(rootPath));
        TEST_ASSERT_TRUE(hostFs->mkdir(nestedPath));
        TEST_ASSERT_TRUE(CreateBinaryFile(filePath, "asset"));

        std::unique_ptr<IFileSystem> rootedFs = std::make_unique<NativeFSImpl>(rootPath);
        TEST_ASSERT_NOT_NULL(rootedFs.get());

        TEST_ASSERT_TRUE(rootedFs->exists("C:\\nested\\asset.txt"));
        TEST_ASSERT_TRUE(rootedFs->exists("nested/asset.txt"));

#if defined(_WIN32)
        TEST_ASSERT_TRUE(hostFs->exists(rootPath + "/nested/asset.txt"));
#else
        TEST_ASSERT_TRUE(hostFs->exists(rootPath + "\\nested\\asset.txt"));
#endif

        RemoveTree(rootPath);
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
        std::unique_ptr<IFileSystem> fs = std::make_unique<NativeFSImpl>();
        TEST_ASSERT_NOT_NULL(fs.get());
        DefaultFileLocator locator(*fs);

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
        RUN_TEST(test_native_file_factory_can_open_and_read_temp_file);
        RUN_TEST(test_native_file_factory_supports_directory_handles);
        RUN_TEST(test_native_file_factory_can_write_and_reopen_file);
        RUN_TEST(test_native_file_factory_chroots_access_and_exposes_virtual_paths);
        RUN_TEST(test_native_file_factory_normalizes_host_and_virtual_path_mangling);
        RUN_TEST(test_content_types_accept_string_view_inputs);
        RUN_TEST(test_default_file_locator_accepts_string_view_configuration);
        return UNITY_END();
    }
}

int run_test_filesystem_posix()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "filesystem native",
        runUnitySuite,
        localSetUp,
        localTearDown);
}