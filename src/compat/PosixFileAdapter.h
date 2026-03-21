#pragma once

#include "FileSystem.h"
#include <cstdio>
#include <sys/stat.h>
#include <string>

namespace HttpServerAdvanced {
namespace Compat {

class PosixFileImpl : public FileImpl {
public:
    PosixFileImpl(FILE* f, std::string name, std::size_t sz, uint32_t lastWrite)
        : file_(f), name_(std::move(name)), size_(sz), lastWrite_(lastWrite) {}

    ~PosixFileImpl() override { if (file_) fclose(file_); }

    int available() override {
        if (!file_) return 0;
        long cur = ftell(file_);
        if (cur < 0) return 0;
        return static_cast<int>(size_ - static_cast<std::size_t>(cur));
    }

    int read() override {
        if (!file_) return -1;
        int c = fgetc(file_);
        if (c == EOF) return -1;
        return c & 0xff;
    }

    int peek() override {
        if (!file_) return -1;
        int c = fgetc(file_);
        if (c == EOF) return -1;
        ungetc(c, file_);
        return c & 0xff;
    }

    bool isValid() const override { return file_ != nullptr; }
    bool isDirectory() const override { return false; }
    void close() override { if (file_) { fclose(file_); file_ = nullptr; } }
    std::size_t size() const override { return size_; }
    const char* fullName() const override { return name_.c_str(); }
    uint32_t getLastWrite() const override { return lastWrite_; }

private:
    FILE* file_ = nullptr;
    std::string name_;
    std::size_t size_ = 0;
    uint32_t lastWrite_ = 0;
};

class PosixFS : public FS {
public:
    File open(const char* path, const char* mode) override {
        if (!path) return File();
        FILE* f = fopen(path, mode);
        if (!f) return File();
        struct stat st;
        uint32_t lastWrite = 0;
        std::size_t sz = 0;
        if (stat(path, &st) == 0) {
            sz = static_cast<std::size_t>(st.st_size);
            lastWrite = static_cast<uint32_t>(st.st_mtime);
        }
        return File(std::make_shared<PosixFileImpl>(f, std::string(path), sz, lastWrite));
    }
};

} // namespace Compat
} // namespace HttpServerAdvanced
