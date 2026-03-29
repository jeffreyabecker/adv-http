#pragma once
#include "../compat/IFileSystem.h"
#include "../compat/ByteStream.h"

#include "../core/HttpContentTypes.h"
#include "../core/HttpContext.h"
#include "../handlers/IHandlerProvider.h"
#include "../response/HttpResponse.h"
#include "FileLocator.h"


#include <memory>
#include <optional>
#include <string>

namespace HttpServerAdvanced {
class StaticFileHandlerFactory : public IHandlerProvider {
private:
  HttpServerAdvanced::HttpContentTypes &contentTypes_;
  std::shared_ptr<FileLocator> ownedFileLocator_{};
  FileLocator *fileLocator_ = nullptr;

  static std::optional<std::string> getEtag(const IFile &file);
  static std::optional<std::string> getLastWriteValue(const IFile &file);

public:
  StaticFileHandlerFactory(FileLocator &fileLocator,
                           HttpServerAdvanced::HttpContentTypes &contentTypes);
  StaticFileHandlerFactory(std::shared_ptr<FileLocator> fileLocator,
                           HttpServerAdvanced::HttpContentTypes &contentTypes);
  bool canHandle(HttpContext &context);
  std::unique_ptr<IHttpHandler> create(HttpContext &context) override;
  void setFileLocator(FileLocator &fileLocator);
};

} // namespace HttpServerAdvanced
