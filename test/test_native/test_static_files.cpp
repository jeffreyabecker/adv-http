#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/httpadv/v1/compat/IFileSystem.h"
#include "../../src/httpadv/v1/core/HttpContentTypes.h"
#include "../../src/httpadv/v1/core/HttpHeader.h"
#include "../../src/httpadv/v1/core/HttpContext.h"
#include "../../src/httpadv/v1/handlers/HttpHandler.h"
#include "../../src/httpadv/v1/staticfiles/AggregateFileLocator.h"
#include "../../src/httpadv/v1/staticfiles/DefaultFileLocator.h"
#include "../../src/httpadv/v1/staticfiles/FileLocator.h"
#include "../../src/httpadv/v1/staticfiles/StaticFileHandler.h"
#include "../../src/httpadv/v1/staticfiles/StaticFilesBuilder.h"
#include "../../src/httpadv/v1/server/HttpServerBase.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace httpadv::v1::core;
using namespace httpadv::v1::handlers;
using namespace httpadv::v1::pipeline;
using namespace httpadv::v1::response;
using namespace httpadv::v1::routing;
using namespace httpadv::v1::server;
using namespace httpadv::v1::staticfiles;
using namespace httpadv::v1::transport;
using namespace httpadv::v1::util;
using namespace httpadv::v1::websocket;

namespace
{
    std::string_view BasenameView(std::string_view path)
    {
        const std::size_t separator = path.find_last_of("\\/");
        if (separator == std::string_view::npos)
        {
            return path;
        }

        return path.substr(separator + 1);
    }

    class NoOpHandler : public IHttpHandler
    {
    public:
        HandlerResult handleStep(HttpContext &) override
        {
            return nullptr;
        }

        void handleBodyChunk(HttpContext &, const uint8_t *, std::size_t) override
        {
        }
    };

    class RequestContextHarness
    {
    public:
        RequestContextHarness()
            : server_(std::make_unique<HttpServerBase>(std::make_unique<httpadv::v1::TestSupport::FakeServer>())),
              handler_(std::make_unique<NoOpHandler>()),
              factory_([this](HttpContext &context) -> std::unique_ptr<IHttpHandler>
              {
                  context_ = &context;
                  return std::move(handler_);
              }),
              pipeline_(HttpContext::createPipelineHandler(*server_, factory_))
        {
        }

        void prepare(std::string_view method, std::string_view url, bool complete = true)
        {
            methodStorage_.assign(method.data(), method.size());
            urlStorage_.assign(url.data(), url.size());
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onMessageBegin(methodStorage_.c_str(), 1, 1, urlStorage_));
            if (complete)
            {
                completeHeaders();
            }
        }

        void addHeader(std::string_view name, std::string_view value)
        {
            nameStorage_.assign(name.data(), name.size());
            valueStorage_.assign(value.data(), value.size());
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeader(nameStorage_, valueStorage_));
        }

        void completeHeaders()
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeadersComplete());
        }

        HttpContext &context()
        {
            TEST_ASSERT_NOT_NULL(context_);
            return *context_;
        }

    private:
        std::unique_ptr<HttpServerBase> server_;
        std::unique_ptr<IHttpHandler> handler_;
        httpadv::v1::TestSupport::RecordingRequestHandlerFactory factory_;
        PipelineHandlerPtr pipeline_;
        HttpContext *context_ = nullptr;
        std::string methodStorage_;
        std::string urlStorage_;
        std::string nameStorage_;
        std::string valueStorage_;
    };

    struct FakeFileSpec
    {
        std::string path;
        bool directory = false;
        std::string content;
        std::optional<std::size_t> size;
        std::optional<uint32_t> lastWrite;
    };

    class FakeFile : public IFile
    {
    public:
        explicit FakeFile(FakeFileSpec spec)
            : spec_(std::move(spec))
        {
        }

        AvailableResult available() override
        {
            if (spec_.directory)
            {
                return ExhaustedResult();
            }

            if (position_ >= spec_.content.size())
            {
                return ExhaustedResult();
            }

            return AvailableBytes(spec_.content.size() - position_);
        }

        size_t read(httpadv::v1::util::span<uint8_t> buffer) override
        {
            if (spec_.directory || closed_ || buffer.empty())
            {
                return 0;
            }

            const size_t copied = peek(buffer);
            position_ += copied;
            return copied;
        }

        size_t peek(httpadv::v1::util::span<uint8_t> buffer) override
        {
            if (spec_.directory || closed_ || buffer.empty())
            {
                return 0;
            }

            const size_t copied = (std::min)(buffer.size(), spec_.content.size() - position_);
            for (size_t i = 0; i < copied; ++i)
            {
                buffer[i] = static_cast<uint8_t>(spec_.content[position_ + i]);
            }
            return copied;
        }

        std::size_t write(httpadv::v1::util::span<const uint8_t>) override
        {
            return 0;
        }

        void flush() override
        {
        }

        bool isDirectory() const override
        {
            return spec_.directory;
        }

        void close() override
        {
            closed_ = true;
        }

        std::optional<std::size_t> size() const override
        {
            if (spec_.size.has_value())
            {
                return spec_.size;
            }

            if (spec_.directory)
            {
                return std::nullopt;
            }

            return spec_.content.size();
        }

        std::string_view name() const override
        {
            return BasenameView(spec_.path);
        }

        std::string_view path() const override
        {
            return spec_.path;
        }

        std::optional<uint32_t> lastWriteEpochSeconds() const override
        {
            return spec_.lastWrite;
        }

    private:
        FakeFileSpec spec_;
        size_t position_ = 0;
        bool closed_ = false;
    };

    class FakeFileSystem : public IFileSystem
    {
    public:
        void add(FakeFileSpec spec)
        {
            spec.path = normalizePath(spec.path);
            files_[spec.path] = std::move(spec);
        }

        bool exists(std::string_view path) const override
        {
            return files_.find(normalizePath(path)) != files_.end();
        }

        bool mkdir(std::string_view path) override
        {
            const std::string normalizedPath = normalizePath(path);
            if (normalizedPath.empty())
            {
                return false;
            }

            auto existing = files_.find(normalizedPath);
            if (existing != files_.end())
            {
                return existing->second.directory;
            }

            files_[normalizedPath] = FakeFileSpec { normalizedPath, true, {}, std::nullopt, std::nullopt };
            return true;
        }

        FileHandle open(std::string_view path, FileOpenMode) override
        {
            const std::string normalizedPath = normalizePath(path);
            openLog_.push_back(normalizedPath);
            auto it = files_.find(normalizedPath);
            if (it == files_.end())
            {
                return nullptr;
            }

            return std::make_unique<FakeFile>(it->second);
        }

        bool remove(std::string_view path) override
        {
            const std::string normalizedPath = normalizePath(path);
            auto it = files_.find(normalizedPath);
            if (it == files_.end())
            {
                return false;
            }

            if (it->second.directory)
            {
                const std::string childPrefix = joinPath(normalizedPath, "");
                for (const auto &entry : files_)
                {
                    if (entry.first.size() > childPrefix.size() && entry.first.compare(0, childPrefix.size(), childPrefix) == 0)
                    {
                        return false;
                    }
                }
            }

            files_.erase(it);
            return true;
        }

        bool rename(std::string_view from, std::string_view to) override
        {
            const std::string normalizedFrom = normalizePath(from);
            const std::string normalizedTo = normalizePath(to);
            if (normalizedFrom.empty() || normalizedTo.empty() || normalizedFrom == normalizedTo)
            {
                return false;
            }

            auto source = files_.find(normalizedFrom);
            if (source == files_.end() || files_.find(normalizedTo) != files_.end())
            {
                return false;
            }

            std::vector<std::pair<std::string, FakeFileSpec>> replacements;
            const std::string fromPrefix = joinPath(normalizedFrom, "");
            const std::string toPrefix = joinPath(normalizedTo, "");
            for (const auto &entry : files_)
            {
                if (entry.first == normalizedFrom)
                {
                    FakeFileSpec moved = entry.second;
                    moved.path = normalizedTo;
                    replacements.emplace_back(normalizedTo, std::move(moved));
                    continue;
                }

                if (source->second.directory && entry.first.size() > fromPrefix.size() && entry.first.compare(0, fromPrefix.size(), fromPrefix) == 0)
                {
                    FakeFileSpec moved = entry.second;
                    const std::string suffix = entry.first.substr(fromPrefix.size());
                    moved.path = toPrefix + suffix;
                    replacements.emplace_back(moved.path, std::move(moved));
                }
            }

            if (replacements.empty())
            {
                return false;
            }

            for (const auto &replacement : replacements)
            {
                if (replacement.first != normalizedTo && files_.find(replacement.first) != files_.end())
                {
                    return false;
                }
            }

            if (source->second.directory)
            {
                for (auto it = files_.begin(); it != files_.end();)
                {
                    if (it->first == normalizedFrom || (it->first.size() > fromPrefix.size() && it->first.compare(0, fromPrefix.size(), fromPrefix) == 0))
                    {
                        it = files_.erase(it);
                        continue;
                    }

                    ++it;
                }
            }
            else
            {
                files_.erase(source);
            }

            for (auto &replacement : replacements)
            {
                files_[replacement.first] = std::move(replacement.second);
            }

            return true;
        }

        bool list(std::string_view directoryPath, const DirectoryEntryCallback &callback, bool recursive = false) override
        {
            if (!callback)
            {
                return false;
            }

            const std::string normalizedDirectory = normalizeDirectoryPath(directoryPath);
            auto directoryIt = files_.find(normalizedDirectory);
            if (directoryIt == files_.end() || !directoryIt->second.directory)
            {
                return false;
            }

            std::set<std::string, std::less<>> emittedPaths;
            for (const auto &entry : files_)
            {
                std::string childName;
                std::string childPath;
                bool isDirectory = false;
                if (!tryBuildDirectoryEntry(normalizedDirectory, entry.second, recursive, childName, childPath, isDirectory))
                {
                    continue;
                }

                if (!emittedPaths.insert(childPath).second)
                {
                    continue;
                }

                const DirectoryEntry directoryEntry { childName, childPath, isDirectory };
                if (!callback(directoryEntry))
                {
                    return true;
                }
            }

            return true;
        }

        const std::vector<std::string> &openLog() const
        {
            return openLog_;
        }

    private:
        static bool isPathSeparator(char value)
        {
            return value == '/' || value == '\\';
        }

        static std::string normalizePath(std::string_view path)
        {
            std::string normalized(path);
            while (normalized.size() > 1 && isPathSeparator(normalized.back()))
            {
                normalized.pop_back();
            }

            return normalized;
        }

        static std::string normalizeDirectoryPath(std::string_view path)
        {
            return normalizePath(path);
        }

        static std::string joinPath(std::string_view base, std::string_view name)
        {
            if (base.empty())
            {
                return std::string(name);
            }

            std::string joined(base);
            if (!isPathSeparator(joined.back()))
            {
                joined.push_back('/');
            }

            joined.append(name.data(), name.size());
            return joined;
        }

        static bool tryBuildDirectoryEntry(std::string_view directoryPath,
                                           const FakeFileSpec &spec,
                                           bool recursive,
                                           std::string &childName,
                                           std::string &childPath,
                                           bool &isDirectory)
        {
            if (spec.path == directoryPath || spec.path.size() <= directoryPath.size())
            {
                return false;
            }

            if (spec.path.compare(0, directoryPath.size(), directoryPath) != 0)
            {
                return false;
            }

            const char separator = spec.path[directoryPath.size()];
            if (!isPathSeparator(separator))
            {
                return false;
            }

            const std::size_t childStart = directoryPath.size() + 1;
            const std::size_t nextSeparator = spec.path.find_first_of("/\\", childStart);
            if (!recursive && nextSeparator != std::string::npos)
            {
                childName = spec.path.substr(childStart, nextSeparator - childStart);
                childPath = joinPath(directoryPath, childName);
                isDirectory = true;
                return true;
            }

            if (recursive)
            {
                childName = spec.path.substr(childStart);
                childPath = spec.path;
                isDirectory = spec.directory;
                return true;
            }

            childName = spec.path.substr(childStart);
            childPath = spec.path;
            isDirectory = spec.directory;
            return true;
        }

        std::map<std::string, FakeFileSpec, std::less<>> files_;
        std::vector<std::string> openLog_;
    };

    class RecordingLocator : public FileLocator
    {
    public:
        RecordingLocator(std::string prefix, std::optional<FakeFileSpec> fileSpec = std::nullopt)
            : prefix_(std::move(prefix)),
              fileSpec_(std::move(fileSpec))
        {
        }

        FileHandle getFile(HttpContext &context) override
        {
            ++getFileCount_;
            lastUrl_ = std::string(context.urlView());
            if (!fileSpec_.has_value())
            {
                return nullptr;
            }

            return std::make_unique<FakeFile>(*fileSpec_);
        }

        bool canHandle(std::string_view path) override
        {
            ++canHandleCount_;
            lastPath_ = std::string(path);
            return path.rfind(prefix_, 0) == 0;
        }

        std::size_t canHandleCount() const
        {
            return canHandleCount_;
        }

        std::size_t getFileCount() const
        {
            return getFileCount_;
        }

    private:
        std::string prefix_;
        std::optional<FakeFileSpec> fileSpec_;
        std::size_t canHandleCount_ = 0;
        std::size_t getFileCount_ = 0;
        std::string lastPath_;
        std::string lastUrl_;
    };

    std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string_view body)
    {
        return StringResponse::create(status, std::string(body), {});
    }

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_default_file_locator_resolves_root_query_and_directory_fallback()
    {
        FakeFileSystem fs;
        fs.add({"/www/docs", true, {}, std::nullopt, 1711152000});
        fs.add({"/www/docs/index.html", false, "<h1>docs</h1>", 13, 1711152000});

        DefaultFileLocator locator(fs);
        RequestContextHarness harness;
        harness.prepare("GET", "/docs/?v=1");

        FileHandle file = locator.getFile(harness.context());
        TEST_ASSERT_NOT_NULL(file.get());
        TEST_ASSERT_FALSE(file->isDirectory());
        TEST_ASSERT_EQUAL_STRING("/www/docs/index.html", std::string(file->path()).c_str());

        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(fs.openLog().size()));
        TEST_ASSERT_EQUAL_STRING("/www/docs", fs.openLog()[0].c_str());
        TEST_ASSERT_EQUAL_STRING("/www/docs/index.html", fs.openLog()[1].c_str());
    }

    void test_default_file_locator_prefers_gzip_variants_and_respects_prefix_filters()
    {
        FakeFileSystem fs;
        fs.add({"/assets/static/app.js.gz", false, "gzip-js", 7, 1711152000});

        DefaultFileLocator locator(fs);
        locator.setFilesystemContentRoot("/assets");
        locator.setRequestPathPrefixes("/static", "/static/api");

        TEST_ASSERT_TRUE(locator.canHandle("/static/app.js"));
        TEST_ASSERT_FALSE(locator.canHandle("/static/api/data"));

        RequestContextHarness harness;
        harness.prepare("GET", "/static/app.js");
        FileHandle file = locator.getFile(harness.context());
        TEST_ASSERT_NOT_NULL(file.get());
        TEST_ASSERT_EQUAL_STRING("/assets/static/app.js.gz", std::string(file->path()).c_str());
    }

    void test_aggregate_file_locator_uses_first_matching_file_and_skips_null_results()
    {
        RecordingLocator apiNullLocator("/api", std::nullopt);
        RecordingLocator apiFileLocator("/api", FakeFileSpec{"/virtual/api.json", false, "api", 3, 1711152000});
        RecordingLocator staticLocator("/static", FakeFileSpec{"/virtual/app.js", false, "js", 2, 1711152000});

        AggregateFileLocator locator;
        locator.add(apiNullLocator);
        locator.add(apiFileLocator);
        locator.add(staticLocator);

        RequestContextHarness harness;
        harness.prepare("GET", "/api/items");
        FileHandle file = locator.getFile(harness.context());
        TEST_ASSERT_NOT_NULL(file.get());
        TEST_ASSERT_EQUAL_STRING("/virtual/api.json", std::string(file->path()).c_str());

        TEST_ASSERT_EQUAL_UINT64(1, apiNullLocator.canHandleCount());
        TEST_ASSERT_EQUAL_UINT64(1, apiNullLocator.getFileCount());
        TEST_ASSERT_EQUAL_UINT64(1, apiFileLocator.canHandleCount());
        TEST_ASSERT_EQUAL_UINT64(1, apiFileLocator.getFileCount());
        TEST_ASSERT_EQUAL_UINT64(0, staticLocator.canHandleCount());
    }

    void test_static_file_handler_factory_rejects_non_matching_paths_and_missing_files()
    {
        FakeFileSystem fs;
        DefaultFileLocator locator(fs);
        HttpContentTypes contentTypes;
        StaticFileHandlerFactory factory(locator, contentTypes);

        RequestContextHarness missingHarness;
        missingHarness.prepare("GET", "/missing.txt");
        TEST_ASSERT_FALSE(factory.canHandle(missingHarness.context()));

        locator.setRequestPathPrefixes("/static");
        RequestContextHarness filteredHarness;
        filteredHarness.prepare("GET", "/api/data");
        TEST_ASSERT_FALSE(factory.canHandle(filteredHarness.context()));
    }

    void test_static_file_handler_factory_serves_files_with_content_type_and_metadata_headers()
    {
        FakeFileSystem fs;
        fs.add({"/www/app.js", false, "console.log('ok');", 18, 1711152000});

        DefaultFileLocator locator(fs);
        HttpContentTypes contentTypes;
        StaticFileHandlerFactory factory(locator, contentTypes);

        RequestContextHarness harness;
        harness.prepare("GET", "/app.js");

        TEST_ASSERT_TRUE(factory.canHandle(harness.context()));
        std::unique_ptr<IHttpHandler> handler = factory.create(harness.context());
        const auto response = httpadv::v1::TestSupport::CaptureResponse(handler->handleStep(harness.context()));

        TEST_ASSERT_EQUAL_UINT16(200, response.status.code());
        TEST_ASSERT_EQUAL_STRING("console.log('ok');", response.body.c_str());
        TEST_ASSERT_TRUE(httpadv::v1::TestSupport::FindCapturedHeader(response, HttpHeaderNames::ContentType).has_value());
        TEST_ASSERT_EQUAL_STRING("application/javascript", httpadv::v1::TestSupport::FindCapturedHeader(response, HttpHeaderNames::ContentType)->c_str());
        TEST_ASSERT_EQUAL_STRING("18", httpadv::v1::TestSupport::FindCapturedHeader(response, HttpHeaderNames::ContentLength)->c_str());
        TEST_ASSERT_TRUE(httpadv::v1::TestSupport::FindCapturedHeader(response, HttpHeaderNames::ETag).has_value());
        TEST_ASSERT_TRUE(httpadv::v1::TestSupport::FindCapturedHeader(response, HttpHeaderNames::LastModified).has_value());
    }

    void test_static_file_handler_factory_handles_directory_gzip_and_method_restrictions()
    {
        FakeFileSystem fs;
        fs.add({"/www/site", true, {}, std::nullopt, 1711152000});
        fs.add({"/www/site/index.html.gz", false, "gzipped-index", 12, 1711152000});

        DefaultFileLocator locator(fs);
        HttpContentTypes contentTypes;
        StaticFileHandlerFactory factory(locator, contentTypes);

        RequestContextHarness getHarness;
        getHarness.prepare("GET", "/site");
        std::unique_ptr<IHttpHandler> getHandler = factory.create(getHarness.context());
        const auto getResponse = httpadv::v1::TestSupport::CaptureResponse(getHandler->handleStep(getHarness.context()));

        TEST_ASSERT_EQUAL_UINT16(200, getResponse.status.code());
        TEST_ASSERT_EQUAL_STRING("gzipped-index", getResponse.body.c_str());
        TEST_ASSERT_EQUAL_STRING("text/html", httpadv::v1::TestSupport::FindCapturedHeader(getResponse, HttpHeaderNames::ContentType)->c_str());
        TEST_ASSERT_EQUAL_STRING("gzip", httpadv::v1::TestSupport::FindCapturedHeader(getResponse, HttpHeaderNames::ContentEncoding)->c_str());

        RequestContextHarness headHarness;
        headHarness.prepare("HEAD", "/site");
        std::unique_ptr<IHttpHandler> headHandler = factory.create(headHarness.context());
        const auto headResponse = httpadv::v1::TestSupport::CaptureResponse(headHandler->handleStep(headHarness.context()));
        TEST_ASSERT_EQUAL_UINT16(200, headResponse.status.code());

        RequestContextHarness postHarness;
        postHarness.prepare("POST", "/site");
        std::unique_ptr<IHttpHandler> postHandler = factory.create(postHarness.context());
        const auto postResponse = httpadv::v1::TestSupport::CaptureResponse(postHandler->handleStep(postHarness.context()));
        TEST_ASSERT_EQUAL_UINT16(405, postResponse.status.code());
        TEST_ASSERT_EQUAL_STRING("GET, HEAD", httpadv::v1::TestSupport::FindCapturedHeader(postResponse, HttpHeaderNames::Allow)->c_str());
    }

    void test_static_file_handler_factory_omits_metadata_headers_when_file_data_is_absent()
    {
        FakeFileSystem fs;
        fs.add({"/www/raw.bin", false, "abc", std::nullopt, std::nullopt});

        DefaultFileLocator locator(fs);
        HttpContentTypes contentTypes;
        StaticFileHandlerFactory factory(locator, contentTypes);

        RequestContextHarness harness;
        harness.prepare("GET", "/raw.bin");
        std::unique_ptr<IHttpHandler> handler = factory.create(harness.context());
        const auto response = httpadv::v1::TestSupport::CaptureResponse(handler->handleStep(harness.context()));

        TEST_ASSERT_EQUAL_UINT16(200, response.status.code());
        TEST_ASSERT_EQUAL_STRING("abc", response.body.c_str());
        TEST_ASSERT_FALSE(httpadv::v1::TestSupport::FindCapturedHeader(response, HttpHeaderNames::ETag).has_value());
        TEST_ASSERT_FALSE(httpadv::v1::TestSupport::FindCapturedHeader(response, HttpHeaderNames::LastModified).has_value());
        TEST_ASSERT_EQUAL_STRING("3", httpadv::v1::TestSupport::FindCapturedHeader(response, HttpHeaderNames::ContentLength)->c_str());
    }

    void test_static_file_handler_factory_matcher_scoped_response_filter_only_applies_when_matcher_matches()
    {
        FakeFileSystem fs;
        fs.add({"/www/index.html", false, "<h1>home</h1>", 13, 1711152000});
        fs.add({"/www/app.js", false, "console.log('ok');", 18, 1711152000});

        HttpContentTypes contentTypes;
        std::vector<StaticFileHandlerFactory::ResponseFilterRule> responseRules;
        responseRules.push_back({
            HandlerMatcher("/?.html"),
            [](std::unique_ptr<IHttpResponse> response) -> std::unique_ptr<IHttpResponse>
            {
                response->headers().set("X-Template", "true", true);
                return response;
            }});

        StaticFileHandlerFactory factory(std::make_shared<DefaultFileLocator>(fs), contentTypes, std::move(responseRules));

        RequestContextHarness htmlHarness;
        htmlHarness.prepare("GET", "/index.html");
        htmlHarness.completeHeaders();
        std::unique_ptr<IHttpHandler> htmlHandler = factory.create(htmlHarness.context());
        const auto htmlResponse = httpadv::v1::TestSupport::CaptureResponse(htmlHandler->handleStep(htmlHarness.context()));
        TEST_ASSERT_TRUE(httpadv::v1::TestSupport::FindCapturedHeader(htmlResponse, "X-Template").has_value());

        RequestContextHarness jsHarness;
        jsHarness.prepare("GET", "/app.js");
        jsHarness.completeHeaders();
        std::unique_ptr<IHttpHandler> jsHandler = factory.create(jsHarness.context());
        const auto jsResponse = httpadv::v1::TestSupport::CaptureResponse(jsHandler->handleStep(jsHarness.context()));
        TEST_ASSERT_FALSE(httpadv::v1::TestSupport::FindCapturedHeader(jsResponse, "X-Template").has_value());
    }

    void test_static_file_handler_factory_matcher_scoped_interceptor_only_applies_when_matcher_matches()
    {
        FakeFileSystem fs;
        fs.add({"/www/index.html", false, "<h1>home</h1>", 13, 1711152000});
        fs.add({"/www/app.js", false, "console.log('ok');", 18, 1711152000});

        HttpContentTypes contentTypes;
        std::vector<StaticFileHandlerFactory::InterceptorRule> interceptorRules;
        interceptorRules.push_back({
            HandlerMatcher("/?.html"),
            [](HttpContext &context, IHttpHandler::InvocationCallback next) -> IHttpHandler::HandlerResult
            {
                IHttpHandler::HandlerResult result = next(context);
                if (result.isResponse() && result.response)
                {
                    result.response->headers().set("X-Intercepted", "true", true);
                }
                return result;
            }});

        StaticFileHandlerFactory factory(std::make_shared<DefaultFileLocator>(fs), contentTypes, {}, std::move(interceptorRules));

        RequestContextHarness htmlHarness;
        htmlHarness.prepare("GET", "/index.html");
        htmlHarness.completeHeaders();
        std::unique_ptr<IHttpHandler> htmlHandler = factory.create(htmlHarness.context());
        const auto htmlResponse = httpadv::v1::TestSupport::CaptureResponse(htmlHandler->handleStep(htmlHarness.context()));
        TEST_ASSERT_TRUE(httpadv::v1::TestSupport::FindCapturedHeader(htmlResponse, "X-Intercepted").has_value());

        RequestContextHarness jsHarness;
        jsHarness.prepare("GET", "/app.js");
        jsHarness.completeHeaders();
        std::unique_ptr<IHttpHandler> jsHandler = factory.create(jsHarness.context());
        const auto jsResponse = httpadv::v1::TestSupport::CaptureResponse(jsHandler->handleStep(jsHarness.context()));
        TEST_ASSERT_FALSE(httpadv::v1::TestSupport::FindCapturedHeader(jsResponse, "X-Intercepted").has_value());
    }

    void test_static_file_handler_factory_matcher_scoped_request_predicate_limits_handling()
    {
        FakeFileSystem fs;
        fs.add({"/www/secure/index.html", false, "secure", 6, 1711152000});
        fs.add({"/www/public/index.html", false, "public", 6, 1711152000});

        HttpContentTypes contentTypes;
        std::vector<StaticFileHandlerFactory::RequestPredicateRule> predicateRules;
        predicateRules.push_back({
            HandlerMatcher("/secure/?"),
            [](HttpContext &context)
            {
                return context.headers().exists("X-Allow", "1");
            }});

        StaticFileHandlerFactory factory(std::make_shared<DefaultFileLocator>(fs), contentTypes, {}, {}, std::move(predicateRules));

        RequestContextHarness secureDeniedHarness;
        secureDeniedHarness.prepare("GET", "/secure/index.html");
        secureDeniedHarness.completeHeaders();
        TEST_ASSERT_FALSE(factory.canHandle(secureDeniedHarness.context()));

        RequestContextHarness publicHarness;
        publicHarness.prepare("GET", "/public/index.html");
        publicHarness.completeHeaders();
        TEST_ASSERT_TRUE(factory.canHandle(publicHarness.context()));

        RequestContextHarness secureAllowedHarness;
        secureAllowedHarness.prepare("GET", "/secure/index.html", false);
        secureAllowedHarness.addHeader("X-Allow", "1");
        secureAllowedHarness.completeHeaders();
        TEST_ASSERT_TRUE(factory.canHandle(secureAllowedHarness.context()));
    }

    void test_static_files_builder_supports_string_pattern_convenience_overloads()
    {
        FakeFileSystem fs;
        StaticFilesBuilder builder(fs, nullptr);

        builder.apply(
            "/?.html",
            [](std::unique_ptr<IHttpResponse> response) -> std::unique_ptr<IHttpResponse>
            {
                return response;
            });

        builder.with(
            "/?.html",
            [](HttpContext &context, IHttpHandler::InvocationCallback next) -> IHttpHandler::HandlerResult
            {
                return next(context);
            });

        builder.filterRequest(
            "/?.html",
            [](HttpContext &)
            {
                return true;
            });

        TEST_ASSERT_TRUE(true);
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_default_file_locator_resolves_root_query_and_directory_fallback);
        RUN_TEST(test_default_file_locator_prefers_gzip_variants_and_respects_prefix_filters);
        RUN_TEST(test_aggregate_file_locator_uses_first_matching_file_and_skips_null_results);
        RUN_TEST(test_static_file_handler_factory_rejects_non_matching_paths_and_missing_files);
        RUN_TEST(test_static_file_handler_factory_serves_files_with_content_type_and_metadata_headers);
        RUN_TEST(test_static_file_handler_factory_handles_directory_gzip_and_method_restrictions);
        RUN_TEST(test_static_file_handler_factory_omits_metadata_headers_when_file_data_is_absent);
        RUN_TEST(test_static_file_handler_factory_matcher_scoped_response_filter_only_applies_when_matcher_matches);
        RUN_TEST(test_static_file_handler_factory_matcher_scoped_interceptor_only_applies_when_matcher_matches);
        RUN_TEST(test_static_file_handler_factory_matcher_scoped_request_predicate_limits_handling);
        RUN_TEST(test_static_files_builder_supports_string_pattern_convenience_overloads);
        return UNITY_END();
    }
}

int run_test_static_files()
{
    return httpadv::v1::TestSupport::RunConsolidatedSuite(
        "static files",
        runUnitySuite,
        localSetUp,
        localTearDown);
}