#pragma once
#include <Arduino.h>
#include <FS.h>
#include "../server/HttpRequest.h"


namespace HttpServerAdvanced
{
    using namespace HttpServerAdvanced;

    class FileLocator
    {
    public:
        virtual ~FileLocator() = default;
        virtual File getFile(HttpRequest &context) = 0;
        virtual bool canHandle(const String &path) = 0;
    };

} // namespace HttpServerAdvanced

