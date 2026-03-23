#pragma once

#ifdef ARDUINO

#include <FS.h>

#include "../compat/ArduinoFileSystemAdapter.h"
#include "StaticFilesBuilder.h"

namespace HttpServerAdvanced
{
    inline std::function<void(WebServerBuilder &)> &StaticFiles(fs::FS &filesystem, std::function<void(StaticFilesBuilder &)> setupFunc = nullptr)
    {
        return StaticFiles(Compat::WrapArduinoFileSystem(filesystem), std::move(setupFunc));
    }
}

#endif