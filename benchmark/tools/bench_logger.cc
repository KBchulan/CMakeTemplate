/******************************************************************************
 *
 * @file       bench_logger.cc
 * @brief      Logger 组件基准测试（面向多线程与关键级别延迟）
 *
 * @author     KBchulan
 * @date       2026/03/24
 * @history    重写：移除不适配 MAX_MESSAGE_SIZE=512 的项目，新增 INFO 洪峰下关键级别延迟
 ******************************************************************************/

#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <tools/Logger.hpp>
#include <vector>

using namespace tools;

namespace
{

using Ns = std::chrono::nanoseconds;
using Clock = std::chrono::steady_clock;

constexpr std::size_t kMaxPayloadBytes = global::logger::MAX_MESSAGE_SIZE - 1;
constexpr std::size_t kFloodMessagesPerThread = 10000;
constexpr std::size_t kLatencySamplesPerIter = 2000;

[[nodiscard]] std::string makePayload(std::size_t len)
{
  return std::string(std::min(len, kMaxPayloadBytes), 'X');
}

[[nodiscard]] double percentileNs(std::vector<std::uint64_t>& data, double percentile_ratio)
{
  if (data.empty())
  {
    return 0.0;
  }

  std::sort(data.begin(), data.end());
  const auto position = static_cast<std::size_t>(percentile_ratio * static_cast<double>(data.size() - 1));
  return static_cast<double>(data[position]);
}

template <typename LogFn>
void runInfoFloodLatencyBench(benchmark::State& state, LogFn&& log_fn)
{
  const auto& logger = Logger::getInstance();
  const auto producer_threads = static_cast<std::size_t>(state.range(0));

  double p50_sum = 0.0;
  double p95_sum = 0.0;
  double p99_sum = 0.0;
  double p999_sum = 0.0;

  for (auto benchmark_iteration : state)
  {
    benchmark::DoNotOptimize(benchmark_iteration);
    state.PauseTiming();
    std::vector<std::thread> flooders;
    flooders.reserve(producer_threads);

    for (std::size_t thread_index = 0; thread_index < producer_threads; ++thread_index)
    {
      flooders.emplace_back(
          [&logger, thread_index]()
          {
            for (std::size_t message_index = 0; message_index < kFloodMessagesPerThread; ++message_index)
            {
              logger.info("Flood thread={} seq={}", thread_index, message_index);
            }
          });
    }

    std::vector<std::uint64_t> latencies_ns;
    latencies_ns.reserve(kLatencySamplesPerIter);
    state.ResumeTiming();

    for (std::size_t sample_index = 0; sample_index < kLatencySamplesPerIter; ++sample_index)
    {
      const auto begin = Clock::now();
      log_fn(logger, sample_index);
      const auto end = Clock::now();
      latencies_ns.push_back(static_cast<std::uint64_t>(std::chrono::duration_cast<Ns>(end - begin).count()));
    }

    state.PauseTiming();
    for (auto& worker_thread : flooders)
    {
      worker_thread.join();
    }

    logger.flush();

    p50_sum += percentileNs(latencies_ns, 0.50);
    p95_sum += percentileNs(latencies_ns, 0.95);
    p99_sum += percentileNs(latencies_ns, 0.99);
    p999_sum += percentileNs(latencies_ns, 0.999);

    state.ResumeTiming();
  }

  logger.flush();
  const auto iter_count = static_cast<double>(state.iterations());
  state.counters["p50_ns"] = p50_sum / iter_count;
  state.counters["p95_ns"] = p95_sum / iter_count;
  state.counters["p99_ns"] = p99_sum / iter_count;
  state.counters["p999_ns"] = p999_sum / iter_count;
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kLatencySamplesPerIter));
}

}  // namespace

// 基础吞吐：INFO
static void BM_BasicInfoLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();

  for (auto benchmark_iteration : state)
  {
    benchmark::DoNotOptimize(benchmark_iteration);
    logger.info("basic info: {}", state.iterations());
  }

  logger.flush();
}
BENCHMARK(BM_BasicInfoLogging);

// 各日志级别吞吐
static void BM_LoggingLevels(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto level = static_cast<int>(state.range(0));

  for (auto benchmark_iteration : state)
  {
    benchmark::DoNotOptimize(benchmark_iteration);
    switch (level)
    {
      case 0:
        logger.trace("trace {}", state.iterations());
        break;
      case 1:
        logger.debug("debug {}", state.iterations());
        break;
      case 2:
        logger.info("info {}", state.iterations());
        break;
      case 3:
        logger.warning("warn {}", state.iterations());
        break;
      case 4:
        logger.error("error {}", state.iterations());
        break;
      case 5:
        logger.fatal("fatal {}", state.iterations());
        break;
      default:
        break;
    }
  }

  logger.flush();
}
BENCHMARK(BM_LoggingLevels)->DenseRange(0, 5);

// 格式化复杂度（保留可比性）
static void BM_FormattedLogging(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto format_complexity = static_cast<int>(state.range(0));

  for (auto benchmark_iteration : state)
  {
    benchmark::DoNotOptimize(benchmark_iteration);
    switch (format_complexity)
    {
      case 0:
        logger.info("simple: {}", 42);
        break;
      case 1:
        logger.info("user={} time={} session={}", "test_user", "2026-03-24", 12345);
        break;
      case 2:
        logger.info("complex: u={} ts={} data=[{:.2f},{:.2f},{:.2f}] s={}", "test_user", "2026-03-24 21:00:00", 1.23,
                    4.56, 7.89, "ok");
        break;
      default:
        break;
    }
  }

  logger.flush();
}
BENCHMARK(BM_FormattedLogging)->DenseRange(0, 2);

// 消息长度（严格限制在 MAX_MESSAGE_SIZE 以内）
static void BM_MessageSizeWithinLimit(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  const auto payload_size = static_cast<std::size_t>(state.range(0));
  const auto payload = makePayload(payload_size);

  for (auto benchmark_iteration : state)
  {
    benchmark::DoNotOptimize(benchmark_iteration);
    logger.print("{}", payload);
  }

  logger.flush();
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(payload.size()));
}
BENCHMARK(BM_MessageSizeWithinLimit)->Arg(32)->Arg(128)->Arg(256)->Arg(384)->Arg(511);

// 多线程吞吐（直接用 benchmark 线程模型）
static void BM_MultiThreadInfoThroughput(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  std::uint64_t seq = 0;

  for (auto benchmark_iteration : state)
  {
    benchmark::DoNotOptimize(benchmark_iteration);
    logger.info("mt-info tid={} seq={}", state.thread_index(), seq++);
  }

  logger.flush();
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MultiThreadInfoThroughput)->ThreadRange(1, 8);

// 多线程混合级别吞吐
static void BM_MultiThreadMixedLevels(benchmark::State& state)
{
  const auto& logger = Logger::getInstance();
  std::uint64_t seq = 0;

  for (auto benchmark_iteration : state)
  {
    benchmark::DoNotOptimize(benchmark_iteration);
    const auto thread_index_u64 = static_cast<std::uint64_t>(state.thread_index());
    switch ((thread_index_u64 + seq) % 6ULL)
    {
      case 0:
        logger.trace("mt-mix t={} s={}", state.thread_index(), seq);
        break;
      case 1:
        logger.debug("mt-mix t={} s={}", state.thread_index(), seq);
        break;
      case 2:
        logger.info("mt-mix t={} s={}", state.thread_index(), seq);
        break;
      case 3:
        logger.warning("mt-mix t={} s={}", state.thread_index(), seq);
        break;
      case 4:
        logger.error("mt-mix t={} s={}", state.thread_index(), seq);
        break;
      case 5:
        logger.fatal("mt-mix t={} s={}", state.thread_index(), seq);
        break;
      default:
        break;
    }
    ++seq;
  }

  logger.flush();
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MultiThreadMixedLevels)->ThreadRange(2, 8);

// INFO 洪峰下 ERROR 入队延迟分位数
static void BM_InfoFlood_ErrorEnqueueLatency(benchmark::State& state)
{
  runInfoFloodLatencyBench(state, [](const Logger& logger, std::size_t sample_index)
                           { logger.error("error-under-flood seq={}", sample_index); });
}
BENCHMARK(BM_InfoFlood_ErrorEnqueueLatency)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// INFO 洪峰下 FATAL 入队延迟分位数
static void BM_InfoFlood_FatalEnqueueLatency(benchmark::State& state)
{
  runInfoFloodLatencyBench(state, [](const Logger& logger, std::size_t sample_index)
                           { logger.fatal("fatal-under-flood seq={}", sample_index); });
}
BENCHMARK(BM_InfoFlood_FatalEnqueueLatency)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

BENCHMARK_MAIN();
