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
} // namespace

namespace HttpServerAdvanced {
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
    HttpServerAdvanced::HttpContentTypes &contentTypes)
    : contentTypes_(contentTypes), fileLocator_(&fileLocator) {}

StaticFileHandlerFactory::StaticFileHandlerFactory(
    std::shared_ptr<FileLocator> fileLocator,
    HttpServerAdvanced::HttpContentTypes &contentTypes)
    : contentTypes_(contentTypes), ownedFileLocator_(std::move(fileLocator)),
      fileLocator_(ownedFileLocator_.get()) {}

bool StaticFileHandlerFactory::canHandle(HttpContext &context) {
  if (fileLocator_ == nullptr || !fileLocator_->canHandle(context.urlView())) {
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
  bool isGet = (method == HttpMethod::Get);
  bool isHead = (method == HttpMethod::Head);

  if (!isGet && !isHead) {
    return HttpHandler::create(StringResponse::create(
        HttpStatus::MethodNotAllowed(), "Method Not Allowed",
        {std::move(HttpHeader::Allow("GET, HEAD"))}));
  }

  if (fileLocator_ == nullptr) {
    return nullptr;
  }

  FileHandle file = fileLocator_->getFile(context);
  if (!file) {
    return HttpHandler::create(
        StringResponse::create(HttpStatus::NotFound(), "Not Found", {}));
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

  HttpHeaderCollection headers;
  headers.push_back(HttpHeader::ContentType(contentType));
  if (const std::optional<std::size_t> fileSize = file->size();
      fileSize.has_value()) {
    const std::string contentLength = std::to_string(*fileSize);
    headers.push_back(HttpHeader::ContentLength(contentLength.c_str()));
  }

  if (const std::optional<std::string> etag = getEtag(*file);
      etag.has_value()) {
    headers.push_back(HttpHeader::ETag(std::move(*etag)));
  }

  if (const std::optional<std::string> lastModified = getLastWriteValue(*file);
      lastModified.has_value()) {
    headers.push_back(HttpHeader::LastModified(std::move(*lastModified)));
  }

  if (isGzipped) {
    headers.push_back(HttpHeader::ContentEncoding("gzip"));
  }

  std::unique_ptr<IByteSource> body = std::move(file);
  return HttpHandler::create(std::make_unique<HttpResponse>(
      HttpStatus::Ok(), std::move(body), std::move(headers)));
}

void StaticFileHandlerFactory::setFileLocator(FileLocator &fileLocator) {
  fileLocator_ = &fileLocator;
}

} // namespace HttpServerAdvanced
