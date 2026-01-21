#pragma once

#include <Arduino.h>
#include <unordered_map>
#include <algorithm>

namespace HttpServerAdvanced
{
    class HttpContentTypes
    {
    private:
        std::unordered_map<String, const char *> contentTypes_;

    public:
        static constexpr const char *ServiceName = "HttpContentTypesLookup";
        static constexpr const char *HTML = "text/html";
        static constexpr const char *CSS = "text/css";
        static constexpr const char *JS = "application/javascript";
        static constexpr const char *JSON = "application/json";
        static constexpr const char *PNG = "image/png";
        static constexpr const char *JPG = "image/jpeg";
        static constexpr const char *JPEG = "image/jpeg";
        static constexpr const char *GIF = "image/gif";
        static constexpr const char *SVG = "image/svg+xml";
        static constexpr const char *ICO = "image/x-icon";
        static constexpr const char *TEXT_PLAIN = "text/plain";
        static constexpr const char *XML = "application/xml";
        static constexpr const char *PDF = "application/pdf";
        static constexpr const char *ZIP = "application/zip";
        static constexpr const char *GZ = "application/gzip";
        static constexpr const char *MP3 = "audio/mpeg";
        static constexpr const char *MP4 = "video/mp4";
        static constexpr const char *WEBM = "video/webm";
        static constexpr const char *WASM = "application/wasm";
        static constexpr const char *UNKNOWN = "application/octet-stream";
        static constexpr const char *FORM_URLENCODED = "application/x-www-form-urlencoded";
        static constexpr const char *MULTIPART_FORM_DATA = "multipart/form-data";
        static constexpr const char *MSGPACK = "application/msgpack";
        HttpContentTypes() : contentTypes_(getTable())
        {
        }

        // Returns the MIME type for a given file extension (without dot), or "application/octet-stream" if unknown
        const char *getContentTypeByExtension(const String &extension)
        {
            String extLower = extension;
            extLower.toLowerCase(); // Normalize to lowercase
            const auto &table = contentTypes_;
            auto it = table.find(extLower);
            if (it != table.end())
            {
                return it->second;
            }
            return table.at("unknown"); // Default type for unknown extensions
        }
        const char *getContentTypeFromPath(const char *path)
        {
            const char *dot = strrchr(path, '.');
            if (dot && *(dot + 1))
            {
                String ext = String(dot + 1);
                ext.toLowerCase();
                return getContentTypeByExtension(ext);
            }
            return getContentTypeByExtension("unknown");
        }
        void set(const String &extension, const char *contentType)
        {
            String extLower = extension;
            extLower.toLowerCase();
            contentTypes_[extLower] = contentType;
        }

    private:
        static const std::unordered_map<String, const char *> &getTable()
        {
            static const std::unordered_map<String, const char *> standardTypes = {
                {"html", HTML},
                {"htm", HTML},
                {"css", CSS},
                {"js", JS},
                {"json", JSON},
                {"png", PNG},
                {"jpg", JPG},
                {"jpeg", JPEG},
                {"gif", GIF},
                {"svg", SVG},
                {"ico", ICO},
                {"txt", TEXT_PLAIN},
                {"xml", XML},
                {"pdf", PDF},
                {"zip", ZIP},
                {"gz", GZ},
                {"mp3", MP3},
                {"mp4", MP4},
                {"webm", WEBM},
                {"wasm", WASM},
                {"unknown", UNKNOWN} // Default type for unknown extensions
            };
            return standardTypes;
        }
    };
} // namespace HttpServerAdvanced