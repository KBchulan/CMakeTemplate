# 引入module
include(GenerateExportHeader)

# 获取所有utils文件
file(GLOB_RECURSE SRC_UTILS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# 生成库
add_library(
  utils
  ${SRC_UTILS_FILES}
)

# 设置警告标志
set_warning_flags(utils)

# 生成导出的头文件
generate_export_header(
  utils
  BASE_NAME UTILS
  EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/UtilsExport.hpp
)

# 设置位置无关
set_target_properties(
  utils PROPERTIES
  DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
  POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

# 指定编译选项
target_compile_options(
  utils PUBLIC
  -Wno-interference-size
)

# 指定编译特性
target_compile_features(
  utils PUBLIC
  cxx_std_23
)

# 用于 atomic 16位操作
check_cxx_compiler_flag("-mcx16" COMPILER_SUPPORTS_MCX16)
if(COMPILER_SUPPORTS_MCX16)
  target_compile_options(utils PUBLIC -mcx16)
  target_link_libraries(utils PUBLIC atomic)
endif()

# AddressSanitizer
if(ENABLE_ASAN)
  target_compile_options(utils PUBLIC -fsanitize=address)
  target_link_libraries(utils PUBLIC -fsanitize=address)
endif()

# UndefinedBehaviorSanitizer
if(ENABLE_UBSAN)
  target_compile_options(utils PUBLIC -fsanitize=undefined)
  target_link_libraries(utils PUBLIC -fsanitize=undefined)
endif()

# 包含头文件
target_include_directories(
  utils PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

# 第三方库
target_link_libraries(
  utils PUBLIC
  Boost::url
  ${JSONCPP_LINK_TARGET}
)

# 安装库文件和头文件
install(
  TARGETS utils
  EXPORT CMakeTemplateTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
