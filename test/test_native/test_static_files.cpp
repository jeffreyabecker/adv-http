#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/compat/IFileSystem.h"
#include "../../src/core/HttpContentTypes.h"
#include "../../src/core/HttpHeader.h"
#include "../../src/core/HttpRequest.h"
#include "../../src/handlers/HttpHandler.h"
#include "../../src/staticfiles/AggregateFileLocator.h"
#include "../../src/staticfiles/DefaultFileLocator.h"
#include "../../src/staticfiles/FileLocator.h"
#include "../../src/staticfiles/StaticFileHandler.h"
#include "../../src/server/HttpServerBase.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    class NoOpHandler : public IHttpHandler
    {
    public:
        HandlerResult handleStep(HttpRequest &) override
        {
            return nullptr;
        }

        void handleBodyChunk(HttpRequest &, const uint8_t *, std::size_t) override
        {
        }
    };

    class RequestContextHarness
    {
    public:
        RequestContextHarness()
            : server_(std::make_unique<HttpServerBase>(std::make_unique<TestSupport::FakeServer>())),
              handler_(std::make_unique<NoOpHandler>()),
              factory_([this](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
              {
                  context_ = &context;
                  return std::move(handler_);
              }),
              pipeline_(HttpRequest::createPipelineHandler(*server_, factory_))
        {
            pipeline_->setResponseStreamCallback([](std::unique_ptr<IByteSource>)
            {
            });
        }

        void prepare(std::string_view method, std::string_view url)
        {
            methodStorage_.assign(method.data(), method.size());
            urlStorage_.assign(url.data(), url.size());
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onMessageBegin(methodStorage_.c_str(), 1, 1, urlStorage_));
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeadersComplete());
        }

        HttpRequest &context()
        {
            TEST_ASSERT_NOT_NULL(context_);
            return *context_;
        }

    private:
        std::unique_ptr<HttpServerBase> server_;
        std::unique_ptr<IHttpHandler> handler_;
        TestSupport::RecordingRequestHandlerFactory factory_;
        PipelineHandlerPtr pipeline_;
        HttpRequest *context_ = nullptr;
        std::string methodStorage_;
        std::string urlStorage_;
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

        size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            if (spec_.directory || closed_ || buffer.empty())
            {
                return 0;
            }

            const size_t copied = peek(buffer);
            position_ += copied;
            return copied;
        }

        size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
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

        std::size_t write(HttpServerAdvanced::span<const uint8_t>) override
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
            files_[spec.path] = std::move(spec);
        }

        FileHandle open(std::string_view path, FileOpenMode) override
        {
            openLog_.emplace_back(path);
            auto it = files_.find(std::string(path));
            if (it == files_.end())
            {
                return nullptr;
            }

            return std::make_unique<FakeFile>(it->second);
        }

        const std::vector<std::string> &openLog() const
        {
            return openLog_;
        }

    private:
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

        FileHandle getFile(HttpRequest &context) override
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
        const auto response = TestSupport::CaptureResponse(handler->handleStep(harness.context()));

        TEST_ASSERT_EQUAL_UINT16(200, response.status.code());
        TEST_ASSERT_EQUAL_STRING("console.log('ok');", response.body.c_str());
        TEST_ASSERT_TRUE(TestSupport::FindCapturedHeader(response, HttpHeaderNames::ContentType).has_value());
        TEST_ASSERT_EQUAL_STRING("application/javascript", TestSupport::FindCapturedHeader(response, HttpHeaderNames::ContentType)->c_str());
        TEST_ASSERT_EQUAL_STRING("18", TestSupport::FindCapturedHeader(response, HttpHeaderNames::ContentLength)->c_str());
        TEST_ASSERT_TRUE(TestSupport::FindCapturedHeader(response, HttpHeaderNames::ETag).has_value());
        TEST_ASSERT_TRUE(TestSupport::FindCapturedHeader(response, HttpHeaderNames::LastModified).has_value());
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
        const auto getResponse = TestSupport::CaptureResponse(getHandler->handleStep(getHarness.context()));

        TEST_ASSERT_EQUAL_UINT16(200, getResponse.status.code());
        TEST_ASSERT_EQUAL_STRING("gzipped-index", getResponse.body.c_str());
        TEST_ASSERT_EQUAL_STRING("text/html", TestSupport::FindCapturedHeader(getResponse, HttpHeaderNames::ContentType)->c_str());
        TEST_ASSERT_EQUAL_STRING("gzip", TestSupport::FindCapturedHeader(getResponse, HttpHeaderNames::ContentEncoding)->c_str());

        RequestContextHarness headHarness;
        headHarness.prepare("HEAD", "/site");
        std::unique_ptr<IHttpHandler> headHandler = factory.create(headHarness.context());
        const auto headResponse = TestSupport::CaptureResponse(headHandler->handleStep(headHarness.context()));
        TEST_ASSERT_EQUAL_UINT16(200, headResponse.status.code());

        RequestContextHarness postHarness;
        postHarness.prepare("POST", "/site");
        std::unique_ptr<IHttpHandler> postHandler = factory.create(postHarness.context());
        const auto postResponse = TestSupport::CaptureResponse(postHandler->handleStep(postHarness.context()));
        TEST_ASSERT_EQUAL_UINT16(405, postResponse.status.code());
        TEST_ASSERT_EQUAL_STRING("GET, HEAD", TestSupport::FindCapturedHeader(postResponse, HttpHeaderNames::Allow)->c_str());
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
        const auto response = TestSupport::CaptureResponse(handler->handleStep(harness.context()));

        TEST_ASSERT_EQUAL_UINT16(200, response.status.code());
        TEST_ASSERT_EQUAL_STRING("abc", response.body.c_str());
        TEST_ASSERT_FALSE(TestSupport::FindCapturedHeader(response, HttpHeaderNames::ETag).has_value());
        TEST_ASSERT_FALSE(TestSupport::FindCapturedHeader(response, HttpHeaderNames::LastModified).has_value());
        TEST_ASSERT_EQUAL_STRING("3", TestSupport::FindCapturedHeader(response, HttpHeaderNames::ContentLength)->c_str());
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
        return UNITY_END();
    }
}

int run_test_static_files()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "static files",
        runUnitySuite,
        localSetUp,
        localTearDown);
}