# 各个子模块
if(USE_CORE)
  add_subdirectory(core)
endif()
if(USE_UTILS)
  add_subdirectory(utils)
endif()

# 主程序
add_executable(${PROJECT_NAME} main.cc)
set_warning_flags(${PROJECT_NAME})

# Debug版本设置后缀
set_target_properties(
  ${PROJECT_NAME} PROPERTIES
  DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
)

# 头文件
target_include_directories(
  ${PROJECT_NAME} PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# 第三方库
target_link_libraries(
  ${PROJECT_NAME} PRIVATE
  fmt::fmt
)

# core库
if(USE_CORE)
  target_link_libraries(${PROJECT_NAME} PRIVATE core)
endif()

# utils库
if(USE_UTILS)
  target_link_libraries(${PROJECT_NAME} PRIVATE utils)
endif()

# 安装可执行文件
install(
  TARGETS ${PROJECT_NAME}
  DESTINATION bin
)
