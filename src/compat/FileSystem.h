#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

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
        class FileImpl
        {
        public:
            virtual ~FileImpl() = default;

            virtual int available() = 0;
            virtual int read() = 0;
            virtual int peek() = 0;

            virtual bool isValid() const = 0;
            virtual bool isDirectory() const = 0;
            virtual void close() = 0;
            virtual std::size_t size() const = 0;
            virtual const char *fullName() const = 0;
            virtual uint32_t getLastWrite() const = 0;
        };

        class File
        {
        private:
            std::shared_ptr<FileImpl> impl_;

        public:
            File() = default;
            explicit File(std::shared_ptr<FileImpl> impl) : impl_(std::move(impl)) {}

            template <typename T, typename = std::enable_if_t<std::is_base_of_v<FileImpl, T>>>
            explicit File(std::shared_ptr<T> impl) : impl_(std::move(impl)) {}

            explicit operator bool() const
            {
                return impl_ != nullptr && impl_->isValid();
            }

            int available()
            {
                return impl_ != nullptr ? impl_->available() : 0;
            }

            int read()
            {
                return impl_ != nullptr ? impl_->read() : -1;
            }

            int peek()
            {
                return impl_ != nullptr ? impl_->peek() : -1;
            }

            bool isDirectory() const
            {
                return impl_ != nullptr ? impl_->isDirectory() : false;
            }

            void close()
            {
                if (impl_ != nullptr)
                {
                    impl_->close();
                }
            }

            std::size_t size() const
            {
                return impl_ != nullptr ? impl_->size() : 0;
            }

            const char *fullName() const
            {
                return impl_ != nullptr ? impl_->fullName() : "";
            }

            uint32_t getLastWrite() const
            {
                return impl_ != nullptr ? impl_->getLastWrite() : 0;
            }
        };

        class FS
        {
        public:
            virtual ~FS() = default;

            virtual File open(const char *path, const char *mode) = 0;

            template <typename TPath>
            auto open(const TPath &path, const char *mode) -> decltype(path.c_str(), File())
            {
                return open(path.c_str(), mode);
            }
        };
#endif
    }

    using File = Compat::File;
    using FS = Compat::FS;
}