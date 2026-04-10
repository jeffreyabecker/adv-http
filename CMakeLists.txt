cmake_minimum_required(VERSION 3.25)
project(lumalink_http LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build native tests" ON)

# Prefer an out-of-source build directory called `build`

include(FetchContent)

# Integrate vendored llhttp if present
if(EXISTS ${CMAKE_SOURCE_DIR}/src/llhttp/CMakeLists.txt)
  # Prefer static llhttp on host builds to avoid requiring DLLs at runtime for tests
  set(LLHTTP_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries (.dll/.so)" FORCE)
  set(LLHTTP_BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries (.lib/.a)" FORCE)
  add_subdirectory(src/llhttp)
endif()

# Collect library sources
file(GLOB_RECURSE LUMALINK_HTTP_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/lumalink/http/*.cpp
  ${CMAKE_SOURCE_DIR}/src/lumalink/http/*/*.cpp
)

add_library(lumalink_http STATIC ${LUMALINK_HTTP_SOURCES})

target_include_directories(lumalink_http
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/llhttp/include>
)

# Discover local lumalink-platform source tree (mirrors PlatformIO symlink behavior)
if(EXISTS "${CMAKE_SOURCE_DIR}/../lumalink-platform/src")
  target_include_directories(lumalink_http PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/../lumalink-platform/src>)
elseif(EXISTS "C:/ode/lumalink-platform/src")
  target_include_directories(lumalink_http PUBLIC $<BUILD_INTERFACE:C:/ode/lumalink-platform/src>)
endif()

if(TARGET llhttp::llhttp)
  target_link_libraries(lumalink_http PUBLIC llhttp::llhttp)
endif()

if(WIN32)
  target_link_libraries(lumalink_http PRIVATE ws2_32)
endif()

if(BUILD_TESTS)
  # Unity test framework
  FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.1
  )
  FetchContent_MakeAvailable(unity)
  add_library(unity_native STATIC ${unity_SOURCE_DIR}/src/unity.c)
  target_include_directories(unity_native PUBLIC ${unity_SOURCE_DIR}/src)
  add_library(unity::unity ALIAS unity_native)

  # ArduinoJson: many tests and headers are guarded by LUMALINK_HTTP_ENABLE_ARDUINO_JSON.
  # To avoid an external fetch failure during CI/configure, build the native tests
  # with Arduino JSON disabled by default. If you prefer to enable ArduinoJson,
  # provide `-DUSE_ARDUINOJSON=ON` with a valid FetchContent strategy.
  option(USE_ARDUINOJSON "Fetch and enable ArduinoJson for native tests" ON)
  if(USE_ARDUINOJSON)
    FetchContent_Declare(
      arduinojson
      GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
      GIT_TAG v7.4.2
    )
    FetchContent_MakeAvailable(arduinojson)
    add_library(arduinojson_iface INTERFACE)
    target_include_directories(arduinojson_iface INTERFACE ${arduinojson_SOURCE_DIR}/src)
    add_library(ArduinoJson::ArduinoJson ALIAS arduinojson_iface)
    # Make ArduinoJson headers available to the main library target
    target_link_libraries(lumalink_http PUBLIC ArduinoJson::ArduinoJson)
    # Ensure the public library target sees ArduinoJson enabled
    target_compile_definitions(lumalink_http PUBLIC LUMALINK_HTTP_ENABLE_ARDUINO_JSON=1)
  else()
    # Explicitly compile the library and tests with ArduinoJson disabled
    add_compile_definitions(LUMALINK_HTTP_ENABLE_ARDUINO_JSON=0)
  endif()

  # Native test runner
  file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/test/test_native/*.cpp
    ${CMAKE_SOURCE_DIR}/test/support/**/*.cpp
  )

  add_executable(tests ${TEST_SOURCES})
  if(USE_ARDUINOJSON)
    target_link_libraries(tests PRIVATE lumalink_http unity::unity ArduinoJson::ArduinoJson)
  else()
    target_link_libraries(tests PRIVATE lumalink_http unity::unity)
  endif()
  target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/test/support/include)

  enable_testing()
  add_test(NAME native_tests COMMAND tests)
endif()

message(STATUS "Configured lumalink_http: BUILD_TESTS=${BUILD_TESTS}")
