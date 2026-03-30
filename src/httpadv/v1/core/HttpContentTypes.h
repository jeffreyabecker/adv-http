#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <string_view>

namespace httpadv::v1::core
{
    class HttpContentTypes
    {
    private:
        std::map<std::string, const char *, std::less<>> contentTypes_;

        static std::string NormalizeExtension(std::string_view extension)
        {
            std::string extLower(extension);
            std::transform(extLower.begin(), extLower.end(), extLower.begin(), [](unsigned char value)
                           { return static_cast<char>(std::tolower(value)); });
            return extLower;
        }

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
        const char *getContentTypeByExtension(std::string_view extension) const
        {
            const std::string extLower = NormalizeExtension(extension);
            const auto &table = contentTypes_;
            auto it = table.find(extLower);
            if (it != table.end())
            {
                return it->second;
            }
            return table.at("unknown"); // Default type for unknown extensions
        }

        const char *getContentTypeFromPath(std::string_view path) const
        {
            const std::size_t dot = path.find_last_of('.');
            if (dot != std::string_view::npos && dot + 1 < path.size())
            {
                return getContentTypeByExtension(path.substr(dot + 1));
            }
            return getContentTypeByExtension("unknown");
        }

        void set(std::string_view extension, const char *contentType)
        {
            std::string extLower = NormalizeExtension(extension);
            contentTypes_[extLower] = contentType;
        }

    private:
        static const std::map<std::string, const char *, std::less<>> &getTable()
        {
            static const std::map<std::string, const char *, std::less<>> standardTypes = {
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
