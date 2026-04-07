#include "StaticFileHandler.h"
#include "../handlers/HttpHandler.h"
#include "../response/StringResponse.h"

#include <any>
#include <cstdint>
#include <ctime>

#include <string>
#include <string_view>

namespace {
bool EndsWith(std::string_view value, std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
             0;
}

class DecoratedStaticHandler : public httpadv::v1::handlers::IHttpHandler {
public:
  DecoratedStaticHandler(
      std::unique_ptr<httpadv::v1::handlers::IHttpHandler> innerHandler,
      httpadv::v1::response::IHttpResponse::ResponseFilter responseFilter,
      httpadv::v1::handlers::IHttpHandler::InterceptorCallback interceptor)
      : innerHandler_(std::move(innerHandler)),
        responseFilter_(std::move(responseFilter)),
        interceptor_(std::move(interceptor)) {}

  HandlerResult handleStep(httpadv::v1::core::HttpRequestContext &context) override {
    HandlerResult result = interceptor_
                               ? interceptor_(context, httpadv::v1::handlers::IHttpHandler::InvocationNext(context, [this, &context]() {
                                   return innerHandler_->handleStep(context);
                                 }))
                               : innerHandler_->handleStep(context);

    if (result.isResponse() && responseFilter_) {
      result.response = responseFilter_(std::move(result.response));
    }

    return result;
  }

  void handleBodyChunk(httpadv::v1::core::HttpRequestContext &context,
                       const uint8_t *at, std::size_t length) override {
    innerHandler_->handleBodyChunk(context, at, length);
  }

private:
  std::unique_ptr<httpadv::v1::handlers::IHttpHandler> innerHandler_;
  httpadv::v1::response::IHttpResponse::ResponseFilter responseFilter_;
  httpadv::v1::handlers::IHttpHandler::InterceptorCallback interceptor_;
};
} // namespace

namespace httpadv::v1::staticfiles {
struct StaticFileHandlerFactory::ResolvedRequest {
  bool canHandle = false;
  std::string requestPath;
  FileHandle file;
};

std::optional<std::string>
StaticFileHandlerFactory::getEtag(const IFile &file) {
  const std::optional<std::size_t> size = file.size();
  const std::optional<uint32_t> lastWrite = file.lastWriteEpochSeconds();
  if (!size.has_value() || !lastWrite.has_value()) {
    return std::nullopt;
  }

  const std::size_t nameLen = file.path().size();
  const uint32_t combined = (static_cast<uint32_t>(*size) << 16) |
                            (static_cast<uint32_t>(nameLen) & 0x0000FFFF);
  uint64_t etagValue = (static_cast<uint64_t>(combined) << 32) |
                       static_cast<uint64_t>(*lastWrite);
  char hex[17];
  snprintf(hex, sizeof(hex), "%016llx", etagValue);
  return std::string(hex);
}

std::string StaticFileHandlerFactory::createResolvedRequestItemKey(
    const void *instance) {
  return "StaticFileHandlerFactory::ResolvedRequest::" +
         std::to_string(reinterpret_cast<std::uintptr_t>(instance));
}

std::optional<std::string>
StaticFileHandlerFactory::getLastWriteValue(const IFile &file) {
  const std::optional<uint32_t> lastWrite = file.lastWriteEpochSeconds();
  if (!lastWrite.has_value()) {
    return std::nullopt;
  }

  char buffer[30];
  const time_t lastWriteTime = static_cast<time_t>(*lastWrite);
  struct tm *tm_info = gmtime(&lastWriteTime);
  if (tm_info == nullptr) {
    return std::nullopt;
  }

  strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
  return std::string(buffer);
}

std::unique_ptr<IHttpResponse> StaticFileHandlerFactory::createFileResponse(
    FileHandle file, httpadv::v1::core::HttpContentTypes &contentTypes) {
  if (!file) {
    return nullptr;
  }

  const std::string_view filePath = file->path();
  const bool isGzipped = EndsWith(filePath, ".gz");
  const bool isBrotli = EndsWith(filePath, ".br");

  const char *contentType = nullptr;
  if (isGzipped || isBrotli) {
    contentType = contentTypes.getContentTypeFromPath(
        filePath.substr(0, filePath.size() - 3));
  } else {
    contentType = contentTypes.getContentTypeFromPath(filePath);
  }

  httpadv::v1::core::HttpHeaderCollection headers;
  headers.push_back(httpadv::v1::core::HttpHeader::ContentType(contentType));
  if (const std::optional<std::size_t> fileSize = file->size();
      fileSize.has_value()) {
    const std::string contentLength = std::to_string(*fileSize);
    headers.push_back(
        httpadv::v1::core::HttpHeader::ContentLength(contentLength.c_str()));
  }

  if (const std::optional<std::string> etag = getEtag(*file);
      etag.has_value()) {
    headers.push_back(httpadv::v1::core::HttpHeader::ETag(std::move(*etag)));
  }

  if (const std::optional<std::string> lastModified = getLastWriteValue(*file);
      lastModified.has_value()) {
    headers.push_back(
        httpadv::v1::core::HttpHeader::LastModified(std::move(*lastModified)));
  }

  if (isGzipped) {
    headers.push_back(httpadv::v1::core::HttpHeader::ContentEncoding("gzip"));
  }
  if (isBrotli) {
    headers.push_back(httpadv::v1::core::HttpHeader::ContentEncoding("br"));
  }

  std::unique_ptr<lumalink::platform::buffers::IByteSource> body = std::move(file);
  return std::make_unique<httpadv::v1::response::HttpResponse>(
      httpadv::v1::core::HttpStatus::Ok(), std::move(body),
      std::move(headers));
}

// Public methods
StaticFileHandlerFactory::StaticFileHandlerFactory(
    std::unique_ptr<FileLocator> fileLocator,
    httpadv::v1::core::HttpContentTypes &contentTypes,
    std::vector<ResponseFilterRule> responseFilterRules,
    std::vector<InterceptorRule> interceptorRules,
  std::vector<RequestPredicateRule> requestPredicateRules,
  MissingRequestPathResolver notFoundRequestPathResolver)
    : contentTypes_(contentTypes), fileLocator_(std::move(fileLocator)),
      notFoundRequestPathResolver_(std::move(notFoundRequestPathResolver)),
      resolvedRequestItemKey_(createResolvedRequestItemKey(this)),
      responseFilterRules_(std::move(responseFilterRules)),
      interceptorRules_(std::move(interceptorRules)),
      requestPredicateRules_(std::move(requestPredicateRules)) {}

bool StaticFileHandlerFactory::passesRequestPredicates(HttpRequestContext &context) const {
  for (const RequestPredicateRule &rule : requestPredicateRules_) {
    if (!rule.predicate) {
      continue;
    }
    if (!rule.matcher.canHandle(context)) {
      continue;
    }
    if (!rule.predicate(context)) {
      return false;
    }
  }

  return true;
}

FileHandle StaticFileHandlerFactory::locateFile(
    HttpRequestContext &context, std::string_view requestPath) const {
  if (!fileLocator_) {
    return nullptr;
  }

  return fileLocator_->getFile(context, requestPath);
}

StaticFileHandlerFactory::ResolvedRequest &
StaticFileHandlerFactory::resolveRequest(HttpRequestContext &context) {
  auto &items = context.items();
  auto existing = items.find(resolvedRequestItemKey_);
  if (existing != items.end()) {
    return *std::any_cast<std::shared_ptr<ResolvedRequest> &>(existing->second);
  }

  ResolvedRequest resolvedRequest;
  if (fileLocator_ && fileLocator_->canHandle(context.urlView()) &&
      passesRequestPredicates(context)) {
    resolvedRequest.requestPath = std::string(context.urlView());
    resolvedRequest.file = locateFile(context, resolvedRequest.requestPath);

    if (!resolvedRequest.file && notFoundRequestPathResolver_) {
      std::optional<std::string> fallbackPath =
          notFoundRequestPathResolver_(context);
      if (fallbackPath.has_value()) {
        resolvedRequest.requestPath = std::move(*fallbackPath);
        resolvedRequest.file = locateFile(context, resolvedRequest.requestPath);
      }
    }

    resolvedRequest.canHandle = static_cast<bool>(resolvedRequest.file);
    if (!resolvedRequest.canHandle) {
      resolvedRequest.requestPath.clear();
    }
  }

  auto [inserted, _] = items.emplace(
      resolvedRequestItemKey_,
      std::make_shared<ResolvedRequest>(std::move(resolvedRequest)));
  return *std::any_cast<std::shared_ptr<ResolvedRequest> &>(inserted->second);
}

std::unique_ptr<IHttpHandler> StaticFileHandlerFactory::decorateHandler(
    HttpRequestContext &context,
    std::unique_ptr<IHttpHandler> innerHandler) const {
  if (!innerHandler) {
    return nullptr;
  }

  IHttpResponse::ResponseFilter responseFilter = nullptr;
  for (const ResponseFilterRule &rule : responseFilterRules_) {
    if (!rule.filter || !rule.matcher.canHandle(context)) {
      continue;
    }

    if (!responseFilter) {
      responseFilter = rule.filter;
      continue;
    }

    auto previousFilter = responseFilter;
    auto nextFilter = rule.filter;
    responseFilter =
        [previousFilter, nextFilter](std::unique_ptr<IHttpResponse> response)
        -> std::unique_ptr<IHttpResponse> {
      return nextFilter(previousFilter(std::move(response)));
    };
  }

  IHttpHandler::InterceptorCallback interceptor = nullptr;
  for (const InterceptorRule &rule : interceptorRules_) {
    if (!rule.wrapper || !rule.matcher.canHandle(context)) {
      continue;
    }

    if (!interceptor) {
      interceptor = rule.wrapper;
      continue;
    }

    auto previousInterceptor = interceptor;
    auto nextInterceptor = rule.wrapper;
    interceptor =
        [previousInterceptor, nextInterceptor](httpadv::v1::core::HttpRequestContext &innerContext,
                                               IHttpHandler::InvocationNext next)
        -> IHttpHandler::HandlerResult {
      return nextInterceptor(
          innerContext,
          IHttpHandler::InvocationNext(innerContext, [previousInterceptor, &innerContext, next]() mutable
              -> IHttpHandler::HandlerResult {
            return previousInterceptor(innerContext, next);
          }));
    };
  }

  if (!responseFilter && !interceptor) {
    return innerHandler;
  }

  return std::make_unique<DecoratedStaticHandler>(
      std::move(innerHandler), std::move(responseFilter),
      std::move(interceptor));
}

bool StaticFileHandlerFactory::canHandle(HttpRequestContext &context) {
  return resolveRequest(context).canHandle;
}

std::unique_ptr<IHttpHandler>
StaticFileHandlerFactory::create(HttpRequestContext &context) {
  ResolvedRequest &resolvedRequest = resolveRequest(context);
  if (!resolvedRequest.canHandle) {
    return nullptr;
  }

  const std::string_view method = context.methodView();
  bool isGet = (method == httpadv::v1::core::HttpMethod::Get);
  bool isHead = (method == httpadv::v1::core::HttpMethod::Head);

  if (!isGet && !isHead) {
    return decorateHandler(context, httpadv::v1::handlers::HttpHandler::create(httpadv::v1::response::StringResponse::create(
      httpadv::v1::core::HttpStatus::MethodNotAllowed(), "Method Not Allowed",
      {std::move(httpadv::v1::core::HttpHeader::Allow("GET, HEAD"))})));
  }

  FileHandle file = std::move(resolvedRequest.file);
  if (!file && !resolvedRequest.requestPath.empty()) {
    file = locateFile(context, resolvedRequest.requestPath);
  }
  if (!file) {
    return nullptr;
  }

  return decorateHandler(
      context, httpadv::v1::handlers::HttpHandler::create(
                   createFileResponse(std::move(file), contentTypes_)));
}

void StaticFileHandlerFactory::setFileLocator(
    std::unique_ptr<FileLocator> fileLocator) {
  fileLocator_ = std::move(fileLocator);
}

void StaticFileHandlerFactory::setNotFoundRequestPathResolver(
    MissingRequestPathResolver resolver) {
  notFoundRequestPathResolver_ = std::move(resolver);
}

} // namespace HttpServerAdvanced
