#pragma once
#include <exception>
#include "LumaLinkPlatform.h"
#include "LumaLinkPlatform.h"

#include "../core/HttpContentTypes.h"
#include "../core/HttpRequestContext.h"
#include "../handlers/IHandlerProvider.h"
#include "../routing/HandlerMatcher.h"
#include "../response/HttpResponse.h"
#include "FileLocator.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace lumalink::http::staticfiles
{
  using lumalink::http::core::HttpRequestContext;
  using lumalink::http::handlers::IHandlerProvider;
  using lumalink::http::handlers::IHttpHandler;
  using lumalink::http::response::IHttpResponse;
  using lumalink::http::routing::HandlerMatcher;
  using lumalink::platform::filesystem::FileHandle;
  using lumalink::platform::filesystem::IFile;

  class StaticFileHandlerFactory : public IHandlerProvider
  {
  private:
    class DecoratedHandler;
    struct ResolvedRequest;

  public:
    using MissingRequestPathResolver =
        std::function<std::optional<std::string>(HttpRequestContext &context)>;

    struct ResponseFilterRule
    {
      HandlerMatcher matcher;
      IHttpResponse::ResponseFilter filter;
    };

    struct InterceptorRule
    {
      HandlerMatcher matcher;
      IHttpHandler::InterceptorCallback wrapper;
    };

    struct RequestPredicateRule
    {
      HandlerMatcher matcher;
      IHttpHandler::Predicate predicate;
    };

  private:
    lumalink::http::core::HttpContentTypes &contentTypes_;
    std::unique_ptr<FileLocator> fileLocator_{};
    MissingRequestPathResolver notFoundRequestPathResolver_{};
    std::string resolvedRequestItemKey_;
    std::vector<ResponseFilterRule> responseFilterRules_;
    std::vector<InterceptorRule> interceptorRules_;
    std::vector<RequestPredicateRule> requestPredicateRules_;

    static std::optional<std::string> getEtag(const IFile &file);
    static std::optional<std::string> getLastWriteValue(const IFile &file);
    static std::string createResolvedRequestItemKey(const void *instance);

    bool passesRequestPredicates(HttpRequestContext &context) const;
    FileHandle locateFile(HttpRequestContext &context, std::string_view requestPath) const;
    ResolvedRequest &resolveRequest(HttpRequestContext &context);
    std::unique_ptr<IHttpHandler> decorateHandler(HttpRequestContext &context, std::unique_ptr<IHttpHandler> innerHandler) const;

  public:
    StaticFileHandlerFactory(std::unique_ptr<FileLocator> fileLocator,
                             lumalink::http::core::HttpContentTypes &contentTypes,
                             std::vector<ResponseFilterRule> responseFilterRules = {},
                             std::vector<InterceptorRule> interceptorRules = {},
                             std::vector<RequestPredicateRule> requestPredicateRules = {},
                             MissingRequestPathResolver notFoundRequestPathResolver = nullptr);
    bool canHandle(HttpRequestContext &context) override;
    std::unique_ptr<IHttpHandler> create(HttpRequestContext &context) override;
    void setFileLocator(std::unique_ptr<FileLocator> fileLocator);
    void setNotFoundRequestPathResolver(MissingRequestPathResolver resolver);
    static std::unique_ptr<IHttpResponse>
    createFileResponse(FileHandle file,
                       lumalink::http::core::HttpContentTypes &contentTypes);
  };

} // namespace lumalink::http::staticfiles
