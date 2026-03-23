#pragma once
#include "../compat/IFileSystem.h"
#include <Arduino.h>
#include "../core/HttpRequest.h"


namespace HttpServerAdvanced
{
    using namespace HttpServerAdvanced;

    class FileLocator
    {
    public:
        virtual ~FileLocator() = default;
        virtual FileHandle getFile(HttpRequest &context) = 0;
        virtual bool canHandle(const String &path) = 0;
    };

} // namespace HttpServerAdvanced


