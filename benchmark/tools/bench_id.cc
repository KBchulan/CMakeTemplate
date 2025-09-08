/******************************************************************************
 *
 * @file       bench_id.cc
 * @brief      ID生成器组件性能基准测试
 *
 * @author     KBchulan
 * @date       2025/09/08
 * @history    UUID和雪花算法ID生成器基准测试套件
 ******************************************************************************/

#include <benchmark/benchmark.h>

#include <cstddef>
#include <thread>
#include <tools/Id.hpp>
#include <vector>

using namespace tools;

// 测试1: UUID生成器性能测试
static void BM_UuidGeneration(benchmark::State& state)
{
  for (auto ___ : state)
  {
    auto uuid_result = UuidGenerator::generateUuid();
    if (uuid_result)
    {
      benchmark::DoNotOptimize(uuid_result.value());
    }
  }
}
BENCHMARK(BM_UuidGeneration);

// 测试2: 短UUID生成器性能测试
static void BM_ShortUuidGeneration(benchmark::State& state)
{
  for (auto ___ : state)
  {
    auto uuid_result = UuidGenerator::generateShortUuid();
    if (uuid_result)
    {
      benchmark::DoNotOptimize(uuid_result.value());
    }
  }
}
BENCHMARK(BM_ShortUuidGeneration);

// 测试3: 雪花算法ID生成器性能测试
static void BM_SnowflakeIdGeneration(benchmark::State& state)
{
  for (auto ___ : state)
  {
    auto id_result = SnowflakeIdGenerator::generateId();
    if (id_result)
    {
      benchmark::DoNotOptimize(id_result.value());
    }
  }
}
BENCHMARK(BM_SnowflakeIdGeneration);

// 测试4: 雪花算法字符串ID生成器性能测试
static void BM_SnowflakeIdStringGeneration(benchmark::State& state)
{
  for (auto ___ : state)
  {
    auto id_result = SnowflakeIdGenerator::generateIdString();
    if (id_result)
    {
      benchmark::DoNotOptimize(id_result.value());
    }
  }
}
BENCHMARK(BM_SnowflakeIdStringGeneration);

// 测试5: 多线程UUID生成性能测试
static void BM_UuidGenerationMultiThread(benchmark::State& state)
{
  const auto num_threads = static_cast<std::size_t>(state.range(0));

  for (auto ___ : state)
  {
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i)
    {
      threads.emplace_back(
          []() -> void
          {
            for (int j = 0; j < 1000; ++j)
            {
              auto uuid_result = UuidGenerator::generateUuid();
              if (uuid_result)
              {
                benchmark::DoNotOptimize(uuid_result.value());
              }
            }
          });
    }

    for (auto& thread : threads)
    {
      thread.join();
    }
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_threads) * 1000);
}
BENCHMARK(BM_UuidGenerationMultiThread)->Range(1, 8);

// 测试6: 多线程雪花算法ID生成性能测试
static void BM_SnowflakeIdGenerationMultiThread(benchmark::State& state)
{
  const auto num_threads = static_cast<int>(state.range(0));

  for (auto ___ : state)
  {
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));

    for (int i = 0; i < num_threads; ++i)
    {
      threads.emplace_back(
          []() -> void
          {
            for (int j = 0; j < 1000; ++j)
            {
              auto id_result = SnowflakeIdGenerator::generateId();
              if (id_result)
              {
                benchmark::DoNotOptimize(id_result.value());
              }
            }
          });
    }

    for (auto& thr : threads)
    {
      thr.join();
    }
  }

  state.SetItemsProcessed(state.iterations() * num_threads * 1000);
}
BENCHMARK(BM_SnowflakeIdGenerationMultiThread)->Range(1, 8);

// 测试7: UUID去重性能测试
static void BM_UuidUniqueness(benchmark::State& state)
{
  std::set<std::string> unique_uuids;
  const auto batch_size = static_cast<int>(state.range(0));

  for (auto ___ : state)
  {
    unique_uuids.clear();
    for (int i = 0; i < batch_size; ++i)
    {
      auto uuid_result = UuidGenerator::generateUuid();
      if (uuid_result)
      {
        unique_uuids.insert(uuid_result.value());
      }
    }
    benchmark::DoNotOptimize(unique_uuids.size());
  }

  state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_UuidUniqueness)->Range(100, 10000);

// 测试8: 雪花算法ID去重性能测试
static void BM_SnowflakeIdUniqueness(benchmark::State& state)
{
  std::set<uint64_t> unique_ids;
  const int batch_size = static_cast<int>(state.range(0));

  for (auto ___ : state)
  {
    unique_ids.clear();
    for (int i = 0; i < batch_size; ++i)
    {
      auto id_result = SnowflakeIdGenerator::generateId();
      if (id_result)
      {
        unique_ids.insert(id_result.value());
      }
    }
    benchmark::DoNotOptimize(unique_ids.size());
  }

  state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_SnowflakeIdUniqueness)->Range(100, 10000);

BENCHMARK_MAIN();
