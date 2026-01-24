#pragma once

#include <Arduino.h>
#include <map>
#include <algorithm>

namespace HttpServerAdvanced
{
    class HttpContentTypes
    {
    private:
        std::map<String, const char *> contentTypes_;

    public:
        static constexpr const char *ServiceName = "HttpContentTypesLookup";
        static constexpr const char *Html = "text/html";
        static constexpr const char *Css = "text/css";
        static constexpr const char *Js = "application/javascript";
        static constexpr const char *Json = "application/json";
        static constexpr const char *Png = "image/png";
        static constexpr const char *Jpg = "image/jpeg";
        static constexpr const char *Jpeg = "image/jpeg";
        static constexpr const char *Gif = "image/gif";
        static constexpr const char *Svg = "image/svg+xml";
        static constexpr const char *Ico = "image/x-icon";
        static constexpr const char *TextPlain = "text/plain";
        static constexpr const char *Xml = "application/xml";
        static constexpr const char *Pdf = "application/pdf";
        static constexpr const char *Zip = "application/zip";
        static constexpr const char *Gz = "application/gzip";
        static constexpr const char *Mp3 = "audio/mpeg";
        static constexpr const char *Mp4 = "video/mp4";
        static constexpr const char *Webm = "video/webm";
        static constexpr const char *Wasm = "application/wasm";
        static constexpr const char *Unknown = "application/octet-stream";
        static constexpr const char *FormUrlencoded = "application/x-www-form-urlencoded";
        static constexpr const char *MultipartFormData = "multipart/form-data";
        static constexpr const char *Msgpack = "application/msgpack";
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
        static const std::map<String, const char *> &getTable()
        {
            static const std::map<String, const char *> standardTypes = {
                {"html", Html},
                {"htm", Html},
                {"css", Css},
                {"js", Js},
                {"json", Json},
                {"png", Png},
                {"jpg", Jpg},
                {"jpeg", Jpeg},
                {"gif", Gif},
                {"svg", Svg},
                {"ico", Ico},
                {"txt", TextPlain},
                {"xml", Xml},
                {"pdf", Pdf},
                {"zip", Zip},
                {"gz", Gz},
                {"mp3", Mp3},
                {"mp4", Mp4},
                {"webm", Webm},
                {"wasm", Wasm},
                {"unknown", Unknown} // Default type for unknown extensions
            };
            return standardTypes;
        }
    };
} // namespace HttpServerAdvanced