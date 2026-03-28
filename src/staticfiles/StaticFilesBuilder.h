#pragma once
#include "../compat/IFileSystem.h"
#include "../core/HttpContentTypes.h"
#include "../routing/HandlerProviderRegistry.h"
#include "../server/WebServerBuilder.h"
#include "DefaultFileLocator.h"
#include "FileLocator.h"
#include "StaticFileHandler.h"


#include <memory>
#include <string>
#include <string_view>

namespace HttpServerAdvanced {

class StaticFilesBuilder {
private:
  std::function<void(StaticFilesBuilder &)> setupFunc_;
  DefaultFileLocator fileLocator_;

  friend std::function<void(WebServerBuilder &)> &
  StaticFiles(IFileSystem &fs,
              std::function<void(StaticFilesBuilder &)> setupFunc);

protected:
  static constexpr const char *NAME = "StaticFiles";
  void init(HttpServerAdvanced::WebServerBuilder &coreBuilder);

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
};

std::function<void(WebServerBuilder &)> &
StaticFiles(IFileSystem &fs,
            std::function<void(StaticFilesBuilder &)> setupFunc = nullptr);
}; // namespace HttpServerAdvanced
