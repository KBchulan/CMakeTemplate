function(add_benchmark BENCHMARK_NAME BENCHMARK_SOURCE)
  add_executable(${BENCHMARK_NAME} ${BENCHMARK_SOURCE})

  target_include_directories(${BENCHMARK_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
  )

  target_link_libraries(${BENCHMARK_NAME}
    PRIVATE
    benchmark::benchmark
    ${ARGN}
  )

  target_compile_features(${BENCHMARK_NAME} PRIVATE cxx_std_23)
  set_warning_flags(${BENCHMARK_NAME})

  if(ENABLE_ASAN)
    target_compile_options(${BENCHMARK_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${BENCHMARK_NAME} PRIVATE -fsanitize=address)
  endif()

  if(ENABLE_UBSAN)
    target_compile_options(${BENCHMARK_NAME} PRIVATE -fsanitize=undefined)
    target_link_options(${BENCHMARK_NAME} PRIVATE -fsanitize=undefined)
  endif()

  target_compile_definitions(${BENCHMARK_NAME}
    PRIVATE
    BENCHMARK_STATIC_DEFINE
  )
endfunction()

######## 添加基准测试 ########

# Logger基准测试
add_benchmark(bench_logger tools/bench_logger.cc fmt::fmt)

# Id生成的基准测试
add_benchmark(bench_id tools/bench_id.cc)