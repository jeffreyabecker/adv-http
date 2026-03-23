#include "../support/include/ConsolidatedNativeSuite.h"

#include <cstdio>

namespace HttpServerAdvanced::TestSupport
{
    namespace
    {
        SuiteHook activeSetUpHook = nullptr;
        SuiteHook activeTearDownHook = nullptr;
    }

    void SetActiveSuiteHooks(SuiteHook setUpHook, SuiteHook tearDownHook)
    {
        activeSetUpHook = setUpHook;
        activeTearDownHook = tearDownHook;
    }

    SuiteHook GetActiveSuiteSetUp()
    {
        return activeSetUpHook;
    }

    SuiteHook GetActiveSuiteTearDown()
    {
        return activeTearDownHook;
    }

    int RunConsolidatedSuite(const char *suiteName, SuiteRunner runner, SuiteHook setUpHook, SuiteHook tearDownHook)
    {
        std::printf("\n== %s ==\n", suiteName);
        SetActiveSuiteHooks(setUpHook, tearDownHook);
        const int failureCount = runner();
        SetActiveSuiteHooks(nullptr, nullptr);
        return failureCount;
    }
}

void setUp()
{
    auto hook = HttpServerAdvanced::TestSupport::GetActiveSuiteSetUp();
    if (hook != nullptr)
    {
        hook();
    }
}

void tearDown()
{
    auto hook = HttpServerAdvanced::TestSupport::GetActiveSuiteTearDown();
    if (hook != nullptr)
    {
        hook();
    }
}

int run_test_filesystem_posix();
int run_test_native_smoke();
int run_test_clock();
int run_test_fixture_support();
int run_test_http_request();
int run_test_request_parser();
int run_test_response_streams();
int run_test_stream_available();
int run_test_stream_utilities();
int run_test_utilities();

int main(int, char **)
{
    int failureCount = 0;

    failureCount += run_test_native_smoke();
    failureCount += run_test_clock();
    failureCount += run_test_fixture_support();
    failureCount += run_test_http_request();
    failureCount += run_test_request_parser();
    failureCount += run_test_response_streams();
    failureCount += run_test_stream_available();
    failureCount += run_test_stream_utilities();
    failureCount += run_test_filesystem_posix();
    failureCount += run_test_utilities();

    return failureCount;
}