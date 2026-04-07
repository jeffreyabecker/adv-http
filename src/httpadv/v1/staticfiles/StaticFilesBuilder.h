#pragma once
#include "../transport/IFileSystem.h"
#include "../core/HttpContentTypes.h"
#include "../routing/HandlerProviderRegistry.h"
#include "../server/WebServerBuilder.h"
#include "DefaultFileLocator.h"
#include "FileLocator.h"
#include "StaticFileHandler.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lumalink::http::staticfiles
{

  using lumalink::http::handlers::IHttpHandler;
  using lumalink::http::response::IHttpResponse;
  using lumalink::http::routing::HandlerMatcher;
  using lumalink::http::server::WebServerBuilder;
  using lumalink::http::transport::IFileSystem;

  class StaticFilesBuilder
  {
  private:
    std::function<void(StaticFilesBuilder &)> setupFunc_;
    IFileSystem *filesystem_ = nullptr;
    std::unique_ptr<FileLocator> fileLocator_;
    StaticFileHandlerFactory::MissingRequestPathResolver notFoundRequestPathResolver_;
    std::vector<std::function<void(DefaultFileLocator &)>> defaultFileLocatorActions_;
    std::vector<StaticFileHandlerFactory::ResponseFilterRule> responseFilterRules_;
    std::vector<StaticFileHandlerFactory::InterceptorRule> interceptorRules_;
    std::vector<StaticFileHandlerFactory::RequestPredicateRule> requestPredicateRules_;

    friend std::function<void(WebServerBuilder &)> &
    StaticFiles(IFileSystem &fs,
                std::function<void(StaticFilesBuilder &)> setupFunc);

  protected:
    static constexpr const char *NAME = "StaticFiles";
    void init(lumalink::http::server::WebServerBuilder &coreBuilder);

  public:
    StaticFilesBuilder(IFileSystem &fs,
                       std::function<void(StaticFilesBuilder &)> setupFunc);
    ~StaticFilesBuilder();
    StaticFilesBuilder &
    setPathPredicate(std::function<bool(std::string_view)> predicate);
    StaticFilesBuilder &
    setPathMapper(std::function<std::string(std::string_view)> mapper);
    StaticFilesBuilder &
    setRequestPathPrefixes(std::string_view prefix = "/",
                           std::string_view excludePrefix = "/api");
    StaticFilesBuilder &setFilesystemContentRoot(std::string_view root);
    StaticFilesBuilder &setFileLocator(std::unique_ptr<FileLocator> locator);
    StaticFilesBuilder &onNotFound(std::string_view requestPath);
    StaticFilesBuilder &onNotFound(StaticFileHandlerFactory::MissingRequestPathResolver resolver);
    StaticFilesBuilder &apply(HandlerMatcher matcher, IHttpResponse::ResponseFilter filter);
    StaticFilesBuilder &apply(const char *uriPattern, IHttpResponse::ResponseFilter filter);
    StaticFilesBuilder &apply(std::string_view uriPattern, IHttpResponse::ResponseFilter filter);
    StaticFilesBuilder &with(HandlerMatcher matcher, IHttpHandler::InterceptorCallback wrapper);
    StaticFilesBuilder &with(const char *uriPattern, IHttpHandler::InterceptorCallback wrapper);
    StaticFilesBuilder &with(std::string_view uriPattern, IHttpHandler::InterceptorCallback wrapper);
    StaticFilesBuilder &filterRequest(HandlerMatcher matcher, IHttpHandler::Predicate predicate);
    StaticFilesBuilder &filterRequest(const char *uriPattern, IHttpHandler::Predicate predicate);
    StaticFilesBuilder &filterRequest(std::string_view uriPattern, IHttpHandler::Predicate predicate);
  };

  std::function<void(WebServerBuilder &)> &
  StaticFiles(IFileSystem &fs,
              std::function<void(StaticFilesBuilder &)> setupFunc = nullptr);
}; // namespace lumalink::http::staticfiles
