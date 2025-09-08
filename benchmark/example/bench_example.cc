#include <benchmark/benchmark.h>
#include <fmt/format.h>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <string>

// 简单的字符串格式化基准测试
static void BM_StringFormat(benchmark::State& state) {
  for (auto ___ : state) {
    std::string result = fmt::format("Hello, {}! Number: {}", "World", 42);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_StringFormat);

// 向量排序基准测试
static void BM_VectorSort(benchmark::State& state) {
  const auto size = static_cast<size_t>(state.range(0));

  for (auto ____ : state) {
    state.PauseTiming();
    std::vector<int> vec(size);
    std::generate(vec.begin(), vec.end(), []() { return rand() % 1000; });
    state.ResumeTiming();

    std::sort(vec.begin(), vec.end());
    benchmark::DoNotOptimize(vec);
  }
}
// 测试不同大小的向量
BENCHMARK(BM_VectorSort)->Range(8, 8<<10);

// 简单的内存分配基准测试
static void BM_MemoryAllocation(benchmark::State& state) {
  const auto size = static_cast<size_t>(state.range(0));

  for (auto ___ : state) {
    auto* ptr = new int[size];
    benchmark::DoNotOptimize(ptr);
    delete[] ptr;
  }
}
BENCHMARK(BM_MemoryAllocation)->Range(1<<10, 1<<20);

BENCHMARK_MAIN();