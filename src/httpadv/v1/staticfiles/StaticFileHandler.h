#pragma once
#include "../compat/IFileSystem.h"
#include "../compat/ByteStream.h"

#include "../core/HttpContentTypes.h"
#include "../core/HttpContext.h"
#include "../handlers/IHandlerProvider.h"
#include "../routing/HandlerMatcher.h"
#include "../response/HttpResponse.h"
#include "FileLocator.h"


#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace httpadv::v1::staticfiles {
using httpadv::v1::core::HttpContext;
using httpadv::v1::handlers::IHandlerProvider;
using httpadv::v1::handlers::IHttpHandler;
using httpadv::v1::response::IHttpResponse;
using httpadv::v1::routing::HandlerMatcher;
using httpadv::v1::transport::IFile;

class StaticFileHandlerFactory : public IHandlerProvider {
private:
  class DecoratedHandler;

public:
  struct ResponseFilterRule {
    HandlerMatcher matcher;
    IHttpResponse::ResponseFilter filter;
  };

  struct InterceptorRule {
    HandlerMatcher matcher;
    IHttpHandler::InterceptorCallback wrapper;
  };

  struct RequestPredicateRule {
    HandlerMatcher matcher;
    IHttpHandler::Predicate predicate;
  };

private:
  httpadv::v1::core::HttpContentTypes &contentTypes_;
  std::shared_ptr<FileLocator> ownedFileLocator_{};
  FileLocator *fileLocator_ = nullptr;
  std::vector<ResponseFilterRule> responseFilterRules_;
  std::vector<InterceptorRule> interceptorRules_;
  std::vector<RequestPredicateRule> requestPredicateRules_;

  static std::optional<std::string> getEtag(const IFile &file);
  static std::optional<std::string> getLastWriteValue(const IFile &file);
  bool passesRequestPredicates(HttpContext &context) const;
  std::unique_ptr<IHttpHandler> decorateHandler(HttpContext &context, std::unique_ptr<IHttpHandler> innerHandler) const;

public:
  StaticFileHandlerFactory(FileLocator &fileLocator,
                           httpadv::v1::core::HttpContentTypes &contentTypes,
                           std::vector<ResponseFilterRule> responseFilterRules = {},
                           std::vector<InterceptorRule> interceptorRules = {},
                           std::vector<RequestPredicateRule> requestPredicateRules = {});
  StaticFileHandlerFactory(std::shared_ptr<FileLocator> fileLocator,
                           httpadv::v1::core::HttpContentTypes &contentTypes,
                           std::vector<ResponseFilterRule> responseFilterRules = {},
                           std::vector<InterceptorRule> interceptorRules = {},
                           std::vector<RequestPredicateRule> requestPredicateRules = {});
  bool canHandle(HttpContext &context) override;
  std::unique_ptr<IHttpHandler> create(HttpContext &context) override;
  void setFileLocator(FileLocator &fileLocator);
};

} // namespace httpadv::v1::staticfiles
