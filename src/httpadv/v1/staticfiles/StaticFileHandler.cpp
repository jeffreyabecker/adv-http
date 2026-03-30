#include "StaticFileHandler.h"
#include "../handlers/HttpHandler.h"
#include "../response/StringResponse.h"
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

  HandlerResult handleStep(httpadv::v1::core::HttpContext &context) override {
    HandlerResult result = interceptor_
                               ? interceptor_(context, [this](httpadv::v1::core::HttpContext &innerContext) {
                                   return innerHandler_->handleStep(innerContext);
                                 })
                               : innerHandler_->handleStep(context);

    if (result.isResponse() && responseFilter_) {
      result.response = responseFilter_(std::move(result.response));
    }

    return result;
  }

  void handleBodyChunk(httpadv::v1::core::HttpContext &context,
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

// Public methods
StaticFileHandlerFactory::StaticFileHandlerFactory(
    FileLocator &fileLocator,
    httpadv::v1::core::HttpContentTypes &contentTypes,
    std::vector<ResponseFilterRule> responseFilterRules,
    std::vector<InterceptorRule> interceptorRules,
    std::vector<RequestPredicateRule> requestPredicateRules)
    : contentTypes_(contentTypes),
      fileLocator_(&fileLocator),
      responseFilterRules_(std::move(responseFilterRules)),
      interceptorRules_(std::move(interceptorRules)),
      requestPredicateRules_(std::move(requestPredicateRules)) {}

StaticFileHandlerFactory::StaticFileHandlerFactory(
    std::shared_ptr<FileLocator> fileLocator,
    httpadv::v1::core::HttpContentTypes &contentTypes,
    std::vector<ResponseFilterRule> responseFilterRules,
    std::vector<InterceptorRule> interceptorRules,
    std::vector<RequestPredicateRule> requestPredicateRules)
    : contentTypes_(contentTypes), ownedFileLocator_(std::move(fileLocator)),
      fileLocator_(ownedFileLocator_.get()),
      responseFilterRules_(std::move(responseFilterRules)),
      interceptorRules_(std::move(interceptorRules)),
      requestPredicateRules_(std::move(requestPredicateRules)) {}

bool StaticFileHandlerFactory::passesRequestPredicates(HttpContext &context) const {
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

std::unique_ptr<IHttpHandler> StaticFileHandlerFactory::decorateHandler(
    HttpContext &context,
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
        [previousInterceptor, nextInterceptor](HttpContext &innerContext,
                                               IHttpHandler::InvocationCallback next)
        -> IHttpHandler::HandlerResult {
      return nextInterceptor(
          innerContext,
          [previousInterceptor, next](HttpContext &chainContext)
              -> IHttpHandler::HandlerResult {
            return previousInterceptor(chainContext, next);
          });
    };
  }

  if (!responseFilter && !interceptor) {
    return innerHandler;
  }

  return std::make_unique<DecoratedStaticHandler>(
      std::move(innerHandler), std::move(responseFilter),
      std::move(interceptor));
}

bool StaticFileHandlerFactory::canHandle(HttpContext &context) {
  if (fileLocator_ == nullptr || !fileLocator_->canHandle(context.urlView())) {
    return false;
  }

  if (!passesRequestPredicates(context)) {
    return false;
  }

  FileHandle file = fileLocator_->getFile(context);
  if (!file) {
    return false;
  }
  file->close();
  return true;
}

std::unique_ptr<IHttpHandler>
StaticFileHandlerFactory::create(HttpContext &context) {
  const std::string_view method = context.methodView();
  bool isGet = (method == httpadv::v1::core::HttpMethod::Get);
  bool isHead = (method == httpadv::v1::core::HttpMethod::Head);

  if (!isGet && !isHead) {
    return decorateHandler(context, httpadv::v1::handlers::HttpHandler::create(httpadv::v1::response::StringResponse::create(
      httpadv::v1::core::HttpStatus::MethodNotAllowed(), "Method Not Allowed",
      {std::move(httpadv::v1::core::HttpHeader::Allow("GET, HEAD"))})));
  }

  if (fileLocator_ == nullptr) {
    return nullptr;
  }

  FileHandle file = fileLocator_->getFile(context);
  if (!file) {
    return decorateHandler(context, httpadv::v1::handlers::HttpHandler::create(
      httpadv::v1::response::StringResponse::create(httpadv::v1::core::HttpStatus::NotFound(), "Not Found", {})));
  }

  const std::string_view filePath = file->path();
  const bool isGzipped = EndsWith(filePath, ".gz");

  const char *contentType = nullptr;
  if (isGzipped) {
    contentType = contentTypes_.getContentTypeFromPath(
        filePath.substr(0, filePath.size() - 3));
  } else {
    contentType = contentTypes_.getContentTypeFromPath(filePath);
  }

  httpadv::v1::core::HttpHeaderCollection headers;
  headers.push_back(httpadv::v1::core::HttpHeader::ContentType(contentType));
  if (const std::optional<std::size_t> fileSize = file->size();
      fileSize.has_value()) {
    const std::string contentLength = std::to_string(*fileSize);
    headers.push_back(httpadv::v1::core::HttpHeader::ContentLength(contentLength.c_str()));
  }

  if (const std::optional<std::string> etag = getEtag(*file);
      etag.has_value()) {
    headers.push_back(httpadv::v1::core::HttpHeader::ETag(std::move(*etag)));
  }

  if (const std::optional<std::string> lastModified = getLastWriteValue(*file);
      lastModified.has_value()) {
    headers.push_back(httpadv::v1::core::HttpHeader::LastModified(std::move(*lastModified)));
  }

  if (isGzipped) {
    headers.push_back(httpadv::v1::core::HttpHeader::ContentEncoding("gzip"));
  }

    std::unique_ptr<httpadv::v1::transport::IByteSource> body = std::move(file);
    return decorateHandler(context, httpadv::v1::handlers::HttpHandler::create(std::make_unique<httpadv::v1::response::HttpResponse>(
      httpadv::v1::core::HttpStatus::Ok(), std::move(body), std::move(headers))));
}

void StaticFileHandlerFactory::setFileLocator(FileLocator &fileLocator) {
  fileLocator_ = &fileLocator;
}

} // namespace HttpServerAdvanced
