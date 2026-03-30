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