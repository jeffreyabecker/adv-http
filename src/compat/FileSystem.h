#pragma once

#include <cstddef>
#include <cstdint>

#include "Stream.h"

#ifdef ARDUINO
#include <FS.h>
#endif

namespace HttpServerAdvanced
{
    namespace Compat
    {
#ifdef ARDUINO
        using File = fs::File;
        using FS = fs::FS;
#else
        class File : public Stream
        {
        public:
            virtual ~File() = default;

            virtual explicit operator bool() const = 0;
            virtual bool isDirectory() const = 0;
            virtual void close() = 0;
            virtual std::size_t size() const = 0;
            virtual const char *fullName() const = 0;
            virtual uint32_t getLastWrite() const = 0;
        };

        class FS
        {
        public:
            virtual ~FS() = default;

            virtual File &open(const char *path, const char *mode) = 0;
        };
#endif
    }

    using File = Compat::File;
    using FS = Compat::FS;
}