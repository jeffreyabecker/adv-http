#pragma once

#if defined(_WIN32)
#include "windows/VirtualPathMapperWindows.h"
#else
#include "posix/VirtualPathMapperPosix.h"
#endif
