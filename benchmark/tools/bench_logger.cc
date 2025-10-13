/******************************************************************************
 *
 * @file       bench_logger.cc
 * @brief      Logger组件性能基准测试
 *
 * @author     KBchulan
 * @date       2025/09/08
 * @history    高性能异步日志系统基准测试套件
 ******************************************************************************/

#include <benchmark/benchmark.h>

#include <chrono>
#include <thread>
#include <tools/Logger.hpp>
#include <vector>

using namespace tools;

// 测试1: 基础日志输出性能测试
static void BM_BasicLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();

  for (auto ___ : state)
  {
    logger.info("This is a test message with number: {}", state.iterations());
  }

  logger.flush();
}
BENCHMARK(BM_BasicLogging);

// 测试2: 不同日志级别性能测试
static void BM_LoggingLevels(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto level = static_cast<int>(state.range(0));

  for (auto ___ : state)
  {
    switch (level)
    {
      case 0:
        logger.trace("Trace message: {}", state.iterations());
        break;
      case 1:
        logger.debug("Debug message: {}", state.iterations());
        break;
      case 2:
        logger.info("Info message: {}", state.iterations());
        break;
      case 3:
        logger.warning("Warning message: {}", state.iterations());
        break;
      case 4:
        logger.error("Error message: {}", state.iterations());
        break;
      case 5:
        logger.fatal("Fatal message: {}", state.iterations());
        break;
    }
  }

  logger.flush();
}
BENCHMARK(BM_LoggingLevels)->DenseRange(0, 5);

// 测试3: 格式化字符串性能测试
static void BM_FormattedLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto format_complexity = static_cast<int>(state.range(0));

  for (auto ___ : state)
  {
    switch (format_complexity)
    {
      case 0:
        logger.info("Simple: {}", 42);
        break;
      case 1:
        logger.info("User {} logged in at {} with session {}", "test_user", "2025-09-08", 12345);
        break;
      case 2:
        logger.info("Complex: user={}, timestamp={}, data={{{}, {}, {}}}, status={}", "test_user",
                    "2025-09-08 12:34:56", 1.23, 4.56, 7.89, "SUCCESS");
        break;
    }
  }

  logger.flush();
}
BENCHMARK(BM_FormattedLogging)->DenseRange(0, 2);

// 测试4: 多线程日志性能测试
static void BM_MultiThreadLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto num_threads = static_cast<std::size_t>(state.range(0));

  for (auto ___ : state)
  {
    state.PauseTiming();
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i)
    {
      threads.emplace_back(
          [&logger, i]()
          {
            for (int j = 0; j < 1000; ++j)
            {
              logger.info("Thread {} message {}: processing data", i, j);
            }
          });
    }
    state.ResumeTiming();

    for (auto& thread : threads)
    {
      thread.join();
    }
  }

  logger.flush();
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_threads) * 1000);
}
BENCHMARK(BM_MultiThreadLogging)->Range(1, 8);

// 测试5: 队列容量压力测试
static void BM_QueuePressureTest(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto message_count = static_cast<std::size_t>(state.range(0));

  for (auto ___ : state)
  {
    for (std::size_t i = 0; i < message_count; ++i)
    {
      logger.info("Pressure test message {}: some data here", i);
    }

    auto queue_size = logger.queueSize();
    benchmark::DoNotOptimize(queue_size);
  }

  logger.flush();
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(message_count));
}
BENCHMARK(BM_QueuePressureTest)->Range(1000, 100000);

// 测试6: 带样式的日志性能测试
static void BM_StyledLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();

  for (auto ___ : state)
  {
    logger.print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, "Styled message with color and bold: {}",
                 state.iterations());
  }

  logger.flush();
}
BENCHMARK(BM_StyledLogging);

// 测试7: 批量日志处理性能测试
static void BM_BatchLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto batch_size = static_cast<std::size_t>(state.range(0));

  for (auto ___ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < batch_size; ++i)
    {
      logger.info("Batch message {}: processing item {}", i, i * 2);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    benchmark::DoNotOptimize(duration.count());
  }

  logger.flush();
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch_size));
}
BENCHMARK(BM_BatchLogging)->Range(10, 10000);

// 测试8: 长字符串日志性能测试
static void BM_LongStringLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto string_length = static_cast<std::size_t>(state.range(0));

  std::string long_string(string_length, 'A');

  for (auto ___ : state)
  {
    logger.info("Long string test: {}", long_string);
  }

  logger.flush();
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(string_length));
}
BENCHMARK(BM_LongStringLogging)->Range(100, 10000);

// 测试9: 混合日志级别多线程测试
static void BM_MixedLevelMultiThreadLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto num_threads = static_cast<std::size_t>(state.range(0));

  for (auto ___ : state)
  {
    state.PauseTiming();
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i)
    {
      threads.emplace_back(
          [&logger, i]()
          {
            for (int j = 0; j < 500; ++j)
            {
              switch (j % 6)
              {
                case 0:
                  logger.trace("Thread {} trace {}", i, j);
                  break;
                case 1:
                  logger.debug("Thread {} debug {}", i, j);
                  break;
                case 2:
                  logger.info("Thread {} info {}", i, j);
                  break;
                case 3:
                  logger.warning("Thread {} warning {}", i, j);
                  break;
                case 4:
                  logger.error("Thread {} error {}", i, j);
                  break;
                case 5:
                  logger.fatal("Thread {} fatal {}", i, j);
                  break;
              }
            }
          });
    }
    state.ResumeTiming();

    for (auto& thread : threads)
    {
      thread.join();
    }
  }

  logger.flush();
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_threads) * 500);
}
BENCHMARK(BM_MixedLevelMultiThreadLogging)->Range(2, 8);

BENCHMARK_MAIN();