# 各个子模块
add_subdirectory(core)
add_subdirectory(utils)

# 主程序
add_executable(${PROJECT_NAME} main.cc)
set_warning_flags(${PROJECT_NAME})

# Debug版本设置后缀
set_target_properties(
  ${PROJECT_NAME} PROPERTIES
  DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
)

# AddressSanitizer
if(ENABLE_ASAN)
  target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
  target_link_libraries(${PROJECT_NAME} PRIVATE -fsanitize=address)
endif()

# UndefinedBehaviorSanitizer
if(ENABLE_UBSAN)
  target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
  target_link_libraries(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
endif()

# 头文件
target_include_directories(
  ${PROJECT_NAME} PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# core库
if(USE_CORE)
  set_warning_flags(core)
  target_link_libraries(${PROJECT_NAME} PRIVATE core)
endif()

# utils库
if(USE_UTILS)
  set_warning_flags(utils)
  target_link_libraries(${PROJECT_NAME} PRIVATE utils)
endif()

# 第三方库
target_link_libraries(
  ${PROJECT_NAME} PRIVATE
  fmt::fmt
)

# 安装可执行文件
install(
  TARGETS ${PROJECT_NAME}
  DESTINATION bin
)
