# 指定最小版本
cmake_minimum_required(VERSION 3.25)

# 默认选择 ninja
find_program(NINJA_EXECUTABLE ninja REQUIRED)
set(CMAKE_MAKE_PROGRAM ${NINJA_EXECUTABLE} CACHE FILEPATH "Ninja executable" FORCE)

# 项目配置
project(
  CMakeTemplate
  VERSION 1.0.0
)

# 组件选择
option(USE_CORE "Use core" ON)
option(USE_UTILS "Use utils" ON)

# 静/动态库选择
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# AddressSanitizer
option(ENABLE_ASAN "Enable AddressSanitizer" ON)

# UndefinedBehaviorSanitizer
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" ON)

# 测试选项
option(BUILD_TESTING "Build tests" OFF)

# 代码覆盖率
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# 基准测试
option(BUILD_BENCHMARK "Build benchmark" OFF)

# cppcheck
option(ENABLE_CPPCHECK "Enable cppcheck" OFF)

# 性能分析
option(ENABLE_PROFILING "Enable profiling flags for perf" OFF)

# 构建类型设置
if(ENABLE_COVERAGE)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Code coverage only works with GCC or Clang")
  endif()

  message(STATUS "Code coverage enabled, forcing Debug build with -O0")
  set(CMAKE_BUILD_TYPE "Debug")
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# 根据构建类型设置编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O3 -DNDEBUG)
  message(STATUS "Release build: Using -O3 optimization")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-O0 -g3 -fstack-protector-strong)
  message(STATUS "Debug build: Using -O0 with debug info")
endif()

message(STATUS "Current build type: ${CMAKE_BUILD_TYPE}")

# 各种情况的RunTimePath
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# 生成编译命令
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# DEBUG模式给一个'd'后缀
set(CMAKE_DEBUG_POSTFIX d)

# 配置安装路径
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

# 配置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 生成配置文件
configure_file(
  ${PROJECT_SOURCE_DIR}/include/config/Config.hpp.in
  ${PROJECT_SOURCE_DIR}/include/config/Config.hpp
  @ONLY
)

# 系统自省配置(check_cxx_source_compiles)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

# 包配置帮助器
include(CMakePackageConfigHelpers)

# 打包
include(CPack)
include(InstallRequiredSystemLibraries)

# 代码质量检测(gcc, clang)
function(set_warning_flags target)
  get_target_property(target_type ${target} TYPE)

  if(target_type STREQUAL "INTERFACE_LIBRARY")
    set(visibility INTERFACE)
  else()
    set(visibility PRIVATE)
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${target} ${visibility}
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wshadow
      -Wcast-align
      -Wcast-qual
      -Wunused
      -Woverloaded-virtual
      -Wformat=2
      -Wdouble-promotion
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} ${visibility}
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wshadow
      -Wcast-align
      -Wcast-qual
      -Wunused
      -Woverloaded-virtual
      -Wformat=2
      -Wdouble-promotion
      -Wextra-semi
      -Winconsistent-missing-destructor-override
    )
  endif()
endfunction()

# fmt
find_package(fmt QUIET)
if(NOT fmt_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
  )
  FetchContent_MakeAvailable(fmt)
endif()

# Google Test
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

# benchmark
find_package(benchmark QUIET)
if(NOT benchmark_FOUND)
  include(FetchContent)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests")
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable gtest tests in benchmark")
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.4
  )
  FetchContent_MakeAvailable(benchmark)
endif()

# 代码覆盖率编译和链接选项
if(ENABLE_COVERAGE)
  add_compile_options(--coverage)
  add_link_options(--coverage)
endif()

# 主程序入口
add_subdirectory(src)

# 测试入口
if(BUILD_TESTING)
  include(CTest)
  enable_testing()
  include(CTestConfig.cmake)
  add_subdirectory(tests)
endif()

# 代码覆盖率报告生成目标
if(ENABLE_COVERAGE)
  find_program(LCOV lcov)
  find_program(GENHTML genhtml)

  if(LCOV AND GENHTML)
    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage/logs
      COMMAND ${LCOV} --quiet --directory . --zerocounters 2>${CMAKE_BINARY_DIR}/coverage/logs/zerocounters.log
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      COMMAND ${LCOV} --quiet --directory . --capture --output-file coverage.info --ignore-errors negative,inconsistent,mismatch --rc geninfo_unexecuted_blocks=0 2>${CMAKE_BINARY_DIR}/coverage/logs/capture.log
      COMMAND ${LCOV} --quiet --remove coverage.info '/usr/*' '*/tests/*' '*/_deps/*' '*/build/*' --output-file coverage.info.cleaned --ignore-errors unused 2>${CMAKE_BINARY_DIR}/coverage/logs/remove.log
      COMMAND ${GENHTML} --quiet coverage.info.cleaned --output-directory ${CMAKE_BINARY_DIR}/coverage --ignore-errors unmapped 2>${CMAKE_BINARY_DIR}/coverage/logs/genhtml.log
      COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in: ${CMAKE_BINARY_DIR}/coverage/index.html"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating code coverage report"
    )
  else()
    message(WARNING "lcov or genhtml not found, coverage target will not be available")
  endif()
endif()

# 基准测试入口
if(BUILD_BENCHMARK)
  add_subdirectory(benchmark)
endif()

# cppcheck
if(ENABLE_CPPCHECK)
  find_program(CPPCHECK cppcheck)
  if(CPPCHECK)
    set(CPPCHECK_COMMAND ${CPPCHECK}
      --enable=all
      --check-level=exhaustive
      --inconclusive
      --std=c++23
      --inline-suppr
      --force
      --quiet
      --output-file=${CMAKE_BINARY_DIR}/cppcheck-report.txt
      --suppress='*:*/_deps/*'
      --suppress='*:*/build/_deps/*'
      --suppress='*:*/build/*/_deps/*'
      --suppress=missingIncludeSystem
      --suppress='constParameterCallback:*/benchmark/*'
      --project=${CMAKE_BINARY_DIR}/compile_commands.json
    )
    add_custom_target(
      cppcheck
      COMMAND ${CPPCHECK_COMMAND}
      COMMENT "Running cppcheck"
    )
  else()
    message(WARNING "cppcheck not found, skipping cppcheck target")
  endif()
endif()

# 性能分析配置
if(ENABLE_PROFILING)
  message(STATUS "Profiling enabled. Compiler flags for perf will be added.")
  add_compile_options(-g -fno-omit-frame-pointer)

  # FlameGraph
  find_package(Git QUIET)
  if(NOT Git_FOUND)
    message(WARNING "Git not found, skipping FlameGraph setup")
  else()
    if(NOT EXISTS ${PROJECT_BINARY_DIR}/flamegraph)
      message(STATUS "FlameGraph directory not found. Cloning...")
      include(FetchContent)
      FetchContent_Declare(
        flamegraph
        GIT_REPOSITORY https://github.com/brendangregg/FlameGraph.git
        GIT_TAG master
        SOURCE_DIR ${PROJECT_BINARY_DIR}/flamegraph
      )
      FetchContent_MakeAvailable(flamegraph)
    else()
      message(STATUS "FlameGraph directory already exists. Skipping clone.")
    endif()
  endif()
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/perf)
endif()

# 安装公共头文件
install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# 安装导出目标
install(
  EXPORT CMakeTemplateTargets
  FILE CMakeTemplateTargets.cmake
  DESTINATION lib/cmake/CMakeTemplate
)

# 配置包文件
configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/Config.cmake.in
  ${CMAKE_BINARY_DIR}/CMakeTemplateConfig.cmake
  INSTALL_DESTINATION lib/cmake/CMakeTemplate
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# 配置版本文件
write_basic_package_version_file(
  ${CMAKE_BINARY_DIR}/CMakeTemplateConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# 安装配置文件
install(
  FILES
  ${CMAKE_BINARY_DIR}/CMakeTemplateConfig.cmake
  ${CMAKE_BINARY_DIR}/CMakeTemplateConfigVersion.cmake
  DESTINATION lib/cmake/CMakeTemplate
)

# 打包配置
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/License.txt)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")

