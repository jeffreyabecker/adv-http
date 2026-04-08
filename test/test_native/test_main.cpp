#include "../support/include/ConsolidatedNativeSuite.h"

#include <cstdio>

namespace lumalink::http::TestSupport {
namespace {
SuiteHook activeSetUpHook = nullptr;
SuiteHook activeTearDownHook = nullptr;
} // namespace

void SetActiveSuiteHooks(SuiteHook setUpHook, SuiteHook tearDownHook) {
  activeSetUpHook = setUpHook;
  activeTearDownHook = tearDownHook;
}

SuiteHook GetActiveSuiteSetUp() { return activeSetUpHook; }

SuiteHook GetActiveSuiteTearDown() { return activeTearDownHook; }

int RunConsolidatedSuite(const char *suiteName, SuiteRunner runner,
                         SuiteHook setUpHook, SuiteHook tearDownHook) {
  std::printf("\n== %s ==\n", suiteName);
  SetActiveSuiteHooks(setUpHook, tearDownHook);
  const int failureCount = runner();
  SetActiveSuiteHooks(nullptr, nullptr);
  return failureCount;
}
} // namespace lumalink::http::TestSupport

void setUp() {
  auto hook = lumalink::http::TestSupport::GetActiveSuiteSetUp();
  if (hook != nullptr) {
    hook();
  }
}

void tearDown() {
  auto hook = lumalink::http::TestSupport::GetActiveSuiteTearDown();
  if (hook != nullptr) {
    hook();
  }
}

int run_test_native_smoke();
int run_test_clock();
int run_test_body_handlers();
int run_test_fixture_support();
int run_test_http_context();
int run_test_pipeline();
int run_test_openapi_builder();
int run_test_request_parser();
int run_test_response_streams();
int run_test_routing();
int run_test_static_files();
int run_test_stream_utilities();
int run_test_utilities();
int run_test_renamed_surface();
int run_test_websocket_error_policy();
int run_test_websocket_frame_codec();
int run_test_websocket_context();

int main(int, char **) {
  int failureCount = 0;

  failureCount += run_test_native_smoke();
  failureCount += run_test_clock();
  failureCount += run_test_body_handlers();
  failureCount += run_test_fixture_support();
  failureCount += run_test_http_context();
  failureCount += run_test_pipeline();
  failureCount += run_test_openapi_builder();
  failureCount += run_test_request_parser();
  failureCount += run_test_routing();
  failureCount += run_test_static_files();
  failureCount += run_test_response_streams();
  failureCount += run_test_stream_utilities();
  failureCount += run_test_websocket_error_policy();
  failureCount += run_test_websocket_frame_codec();
  failureCount += run_test_websocket_context();
  failureCount += run_test_utilities();
  failureCount += run_test_renamed_surface();

  return failureCount;
}