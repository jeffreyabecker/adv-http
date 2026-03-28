#include "StaticFilesBuilder.h"

namespace HttpServerAdvanced {
StaticFilesBuilder::StaticFilesBuilder(
    IFileSystem &fs, std::function<void(StaticFilesBuilder &)> setupFunc)
    : setupFunc_(setupFunc), fileLocator_(fs) {}

StaticFilesBuilder::~StaticFilesBuilder() {}

void StaticFilesBuilder::init(
    HttpServerAdvanced::WebServerBuilder &coreBuilder) {
  if (setupFunc_) {
    setupFunc_(*this);
  }
  auto &handlerFactory = coreBuilder.handlerProviders();
  handlerFactory.addAt<StaticFileHandlerFactory>(
      0, std::make_shared<DefaultFileLocator>(fileLocator_),
      coreBuilder.contentTypes());
}

StaticFilesBuilder &StaticFilesBuilder::setPathPredicate(
    std::function<bool(std::string_view)> predicate) {
  fileLocator_.setPathPredicate(predicate);
  return *this;
}

StaticFilesBuilder &StaticFilesBuilder::setPathMapper(
    std::function<std::string(std::string_view)> mapper) {
  fileLocator_.setPathMapper(mapper);
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::setRequestPathPrefixes(std::string_view prefix,
                                           std::string_view excludePrefix) {
  fileLocator_.setRequestPathPrefixes(prefix, excludePrefix);
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::setFilesystemContentRoot(std::string_view root) {
  fileLocator_.setFilesystemContentRoot(root);
  return *this;
}

std::function<void(WebServerBuilder &)> &
StaticFiles(IFileSystem &fs,
            std::function<void(StaticFilesBuilder &)> setupFunc) {
  auto *instance = new std::function<void(WebServerBuilder &)>(
      [setupFunc, &fs](WebServerBuilder &coreBuilder) {
        StaticFilesBuilder staticFilesBuilder(fs, setupFunc);
        staticFilesBuilder.init(coreBuilder);
      });
  return *instance;
}

} // namespace HttpServerAdvanced
