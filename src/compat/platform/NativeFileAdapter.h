#pragma once

#include "../IFileSystem.h"

#if defined(_WIN32)
#include "windows/WindowsFileAdapter.h"

namespace HttpServerAdvanced::Compat::platform
{
    using NativeFS = windows::WindowsFs;
}
#else
#include "posix/PosixFileAdapter.h"

namespace HttpServerAdvanced::Compat::platform
{
    using NativeFS = posix::PosixFS;
}
#endif

