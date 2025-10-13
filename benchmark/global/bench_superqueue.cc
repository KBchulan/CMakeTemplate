/******************************************************************************
 *
 * @file       bench_superqueue.cc
 * @brief      SuperQueue无锁队列性能基准测试
 *
 * @author     KBchulan
 * @date       2025/10/13
 * @history    SuperQueue性能和并发基准测试套件
 ******************************************************************************/

#include <benchmark/benchmark.h>

#include <atomic>
#include <global/SuperQueue.hpp>
#include <string>
#include <thread>
#include <vector>

// 测试1: 单线程入队性能测试
static void BM_SuperQueue_SingleThread_Enqueue(benchmark::State& state)
{
  global::SuperQueue<int, 1024> queue;

  for (auto ___ : state)
  {
    for (int i = 0; i < 512; ++i)
    {
      benchmark::DoNotOptimize(queue.emplace(i));
    }

    int value;
    while (queue.pop(value))
    {
    }
  }

  state.SetItemsProcessed(state.iterations() * 512);
}
BENCHMARK(BM_SuperQueue_SingleThread_Enqueue);

// 测试2: 单线程出队性能测试
static void BM_SuperQueue_SingleThread_Dequeue(benchmark::State& state)
{
  global::SuperQueue<int, 1024> queue;

  for (auto ___ : state)
  {
    for (int i = 0; i < 512; ++i)
    {
      queue.emplace(i);
    }

    int value;
    for (int i = 0; i < 512; ++i)
    {
      benchmark::DoNotOptimize(queue.pop(value));
    }
  }

  state.SetItemsProcessed(state.iterations() * 512);
}
BENCHMARK(BM_SuperQueue_SingleThread_Dequeue);

// 测试3: 单线程入队出队混合性能测试
static void BM_SuperQueue_SingleThread_Mixed(benchmark::State& state)
{
  global::SuperQueue<int, 1024> queue;

  for (auto ___ : state)
  {
    for (int i = 0; i < 256; ++i)
    {
      queue.emplace(i);
      int value;
      queue.pop(value);
      benchmark::DoNotOptimize(value);
    }
  }

  state.SetItemsProcessed(state.iterations() * 256 * 2);
}
BENCHMARK(BM_SuperQueue_SingleThread_Mixed);

// 测试4: 多生产者单消费者性能测试
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void BM_SuperQueue_MultiProducer_SingleConsumer(benchmark::State& state)
{
  const auto num_producers = static_cast<int>(state.range(0));
  global::SuperQueue<int, 4096> queue;

  for (auto ___ : state)
  {
    std::atomic<bool> start{false};
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    std::vector<std::thread> producers;
    producers.reserve(static_cast<size_t>(num_producers));

    for (int i = 0; i < num_producers; ++i)
    {
      producers.emplace_back(
          [&queue, &start, &produced, i]()
          {
            while (!start.load(std::memory_order_acquire))
            {
            }

            for (int j = 0; j < 1000; ++j)
            {
              while (!queue.emplace((i * 1000) + j))
              {
                std::this_thread::yield();
              }
              produced.fetch_add(1, std::memory_order_relaxed);
            }
          });
    }

    std::thread consumer(
        [&queue, &consumed, total = num_producers * 1000]()
        {
          int value;
          while (consumed.load(std::memory_order_relaxed) < total)
          {
            if (queue.pop(value))
            {
              consumed.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
              std::this_thread::yield();
            }
          }
        });

    start.store(true, std::memory_order_release);

    for (auto& producer : producers)
    {
      producer.join();
    }
    consumer.join();

    benchmark::DoNotOptimize(consumed.load());
  }

  state.SetItemsProcessed(state.iterations() * num_producers * 1000);
}
BENCHMARK(BM_SuperQueue_MultiProducer_SingleConsumer)->Range(2, 8);

// 测试5: 单生产者多消费者性能测试
static void BM_SuperQueue_SingleProducer_MultiConsumer(benchmark::State& state)
{
  const auto num_consumers = static_cast<int>(state.range(0));
  global::SuperQueue<int, 4096> queue;

  for (auto ___ : state)
  {
    std::atomic<bool> start{false};
    std::atomic<int> consumed{0};
    const int total_items = num_consumers * 1000;

    std::thread producer(
        [&queue, &start, total_items]()
        {
          while (!start.load(std::memory_order_acquire))
          {
          }

          for (int i = 0; i < total_items; ++i)
          {
            while (!queue.emplace(i))
            {
              std::this_thread::yield();
            }
          }
        });

    std::vector<std::thread> consumers;
    consumers.reserve(static_cast<size_t>(num_consumers));

    for (int i = 0; i < num_consumers; ++i)
    {
      consumers.emplace_back(
          [&queue, &consumed, total_items]()
          {
            int value;
            while (consumed.load(std::memory_order_relaxed) < total_items)
            {
              if (queue.pop(value))
              {
                consumed.fetch_add(1, std::memory_order_relaxed);
              }
              else
              {
                std::this_thread::yield();
              }
            }
          });
    }

    start.store(true, std::memory_order_release);

    producer.join();
    for (auto& consumer : consumers)
    {
      consumer.join();
    }

    benchmark::DoNotOptimize(consumed.load());
  }

  state.SetItemsProcessed(state.iterations() * num_consumers * 1000);
}
BENCHMARK(BM_SuperQueue_SingleProducer_MultiConsumer)->Range(2, 8);

// 测试6: 多生产者多消费者性能测试
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void BM_SuperQueue_MultiProducer_MultiConsumer(benchmark::State& state)
{
  const auto num_threads = static_cast<int>(state.range(0));
  global::SuperQueue<int, 4096> queue;

  for (auto ___ : state)
  {
    std::atomic<bool> start{false};
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    const int items_per_producer = 1000;

    std::vector<std::thread> producers;
    producers.reserve(static_cast<size_t>(num_threads));

    for (int i = 0; i < num_threads; ++i)
    {
      producers.emplace_back(
          [&queue, &start, &produced, i]()
          {
            while (!start.load(std::memory_order_acquire))
            {
            }

            for (int j = 0; j < items_per_producer; ++j)
            {
              while (!queue.emplace((i * items_per_producer) + j))
              {
                std::this_thread::yield();
              }
              produced.fetch_add(1, std::memory_order_relaxed);
            }
          });
    }

    std::vector<std::thread> consumers;
    consumers.reserve(static_cast<size_t>(num_threads));

    for (int i = 0; i < num_threads; ++i)
    {
      consumers.emplace_back(
          [&queue, &consumed, total = num_threads * items_per_producer]()
          {
            int value;
            while (consumed.load(std::memory_order_relaxed) < total)
            {
              if (queue.pop(value))
              {
                consumed.fetch_add(1, std::memory_order_relaxed);
              }
              else
              {
                std::this_thread::yield();
              }
            }
          });
    }

    start.store(true, std::memory_order_release);

    for (auto& producer : producers)
    {
      producer.join();
    }
    for (auto& consumer : consumers)
    {
      consumer.join();
    }

    benchmark::DoNotOptimize(consumed.load());
  }

  state.SetItemsProcessed(state.iterations() * num_threads * 1000);
}
BENCHMARK(BM_SuperQueue_MultiProducer_MultiConsumer)->Range(2, 8);

// 测试7: 复杂类型性能测试
static void BM_SuperQueue_ComplexType(benchmark::State& state)
{
  struct ComplexData
  {
    int id;
    std::string content;
    double value;

    ComplexData(int identifier, std::string cont, double val) : id(identifier), content(std::move(cont)), value(val)
    {
    }
  };

  global::SuperQueue<ComplexData, 512> queue;

  for (auto ___ : state)
  {
    for (int i = 0; i < 256; ++i)
    {
      queue.emplace(i, "test_data_" + std::to_string(i), static_cast<double>(i) * 1.5);
    }

    ComplexData data(0, "", 0.0);
    for (int i = 0; i < 256; ++i)
    {
      queue.pop(data);
      benchmark::DoNotOptimize(data.id);
    }
  }

  state.SetItemsProcessed(state.iterations() * 256 * 2);
}
BENCHMARK(BM_SuperQueue_ComplexType);

// 测试8: 不同容量下的性能测试
template <size_t Capacity>
static void BM_SuperQueue_DifferentCapacity(benchmark::State& state)
{
  global::SuperQueue<int, Capacity> queue;
  const int operations = static_cast<int>(Capacity / 2);

  for (auto ___ : state)
  {
    for (int i = 0; i < operations; ++i)
    {
      queue.emplace(i);
    }

    int value;
    for (int i = 0; i < operations; ++i)
    {
      queue.pop(value);
      benchmark::DoNotOptimize(value);
    }
  }

  state.SetItemsProcessed(state.iterations() * operations * 2);
}
BENCHMARK(BM_SuperQueue_DifferentCapacity<128>);
BENCHMARK(BM_SuperQueue_DifferentCapacity<512>);
BENCHMARK(BM_SuperQueue_DifferentCapacity<2048>);
BENCHMARK(BM_SuperQueue_DifferentCapacity<8192>);

// 测试9: 高竞争场景性能测试
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void BM_SuperQueue_HighContention(benchmark::State& state)
{
  const auto num_threads = static_cast<int>(state.range(0));
  global::SuperQueue<int, 512> queue;

  for (auto ___ : state)
  {
    std::atomic<bool> start{false};
    std::atomic<int> operations{0};

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));

    for (int i = 0; i < num_threads; ++i)
    {
      threads.emplace_back(
          [&queue, &start, &operations, i]()
          {
            while (!start.load(std::memory_order_acquire))
            {
            }

            for (int j = 0; j < 1000; ++j)
            {
              if (j % 2 == 0)
              {
                if (queue.emplace((i * 1000) + j))
                {
                  operations.fetch_add(1, std::memory_order_relaxed);
                }
              }
              else
              {
                int value;
                if (queue.pop(value))
                {
                  operations.fetch_add(1, std::memory_order_relaxed);
                }
              }
            }
          });
    }

    start.store(true, std::memory_order_release);

    for (auto& thread : threads)
    {
      thread.join();
    }

    int value;
    while (queue.pop(value))
    {
      operations.fetch_add(1, std::memory_order_relaxed);
    }

    benchmark::DoNotOptimize(operations.load());
  }

  state.SetItemsProcessed(state.iterations() * num_threads * 1000);
}
BENCHMARK(BM_SuperQueue_HighContention)->Range(4, 16);

BENCHMARK_MAIN();
