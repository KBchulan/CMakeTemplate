# 引入module
include(GenerateExportHeader)

# 获取所有core文件
file(GLOB_RECURSE SRC_CORE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# 生成库
add_library(
  core
  ${SRC_CORE_FILES}
)

# 设置警告标志
set_warning_flags(core)

# 生成导出的头文件
generate_export_header(
  core
  BASE_NAME CORE
  EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/CoreExport.hpp
)

# 设置位置无关
set_target_properties(
  core PROPERTIES
  DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
  POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

# 指定编译选项
target_compile_options(
  core PUBLIC
  -Wno-interference-size
)

# 指定编译特性
target_compile_features(
  core PUBLIC
  cxx_std_23
)

# 用于 atomic 16位操作
check_cxx_compiler_flag("-mcx16" COMPILER_SUPPORTS_MCX16)
if(COMPILER_SUPPORTS_MCX16)
  target_compile_options(core PUBLIC -mcx16)
  target_link_libraries(core PUBLIC atomic)
endif()

# AddressSanitizer
if(ENABLE_ASAN)
  target_compile_options(core PUBLIC -fsanitize=address)
  target_link_libraries(core PUBLIC -fsanitize=address)
endif()

# UndefinedBehaviorSanitizer
if(ENABLE_UBSAN)
  target_compile_options(core PUBLIC -fsanitize=undefined)
  target_link_libraries(core PUBLIC -fsanitize=undefined)
endif()

# 包含头文件
target_include_directories(
  core PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# 安装库文件和头文件
install(
  TARGETS core
  EXPORT CMakeTemplateTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
