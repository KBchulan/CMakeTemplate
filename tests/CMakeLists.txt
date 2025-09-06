function(add_unit_test TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${TEST_SOURCE})

  target_include_directories(${TEST_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
  )

  target_link_libraries(${TEST_NAME}
    PRIVATE
    GTest::gtest_main
    ${ARGN}
  )

  target_compile_features(${TEST_NAME} PRIVATE cxx_std_23)
  set_warning_flags(${TEST_NAME})

  if(ENABLE_ASAN)
    target_compile_options(${TEST_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${TEST_NAME} PRIVATE -fsanitize=address)
  endif()

  if(ENABLE_UBSAN)
    target_compile_options(${TEST_NAME} PRIVATE -fsanitize=undefined)
    target_link_options(${TEST_NAME} PRIVATE -fsanitize=undefined)
  endif()

  include(GoogleTest)
  gtest_discover_tests(${TEST_NAME})
endfunction()

######## 在下面添加新的测试 ########

# Logger单元测试
add_unit_test(test_logger tools/test_logger.cc fmt::fmt)

# Id生成的单元测试
add_unit_test(test_id tools/test_id.cc)
