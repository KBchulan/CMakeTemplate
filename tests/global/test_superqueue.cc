/******************************************************************************
 *
 * @file       test_superqueue.cc
 * @brief      SuperQueue无锁队列单元测试
 *
 * @author     KBchulan
 * @date       2025/10/13
 * @history    SuperQueue边界和并发测试套件
 ******************************************************************************/

#include <gtest/gtest.h>

#include <atomic>
#include <global/SuperQueue.hpp>
#include <string>
#include <thread>
#include <vector>

struct TestMessage
{
  int id{0};
  std::string content;

  TestMessage() = default;
  TestMessage(int _id, std::string _content) : id(_id), content(std::move(_content))
  {
  }
};

class SuperQueueTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
  }

  void TearDown() override
  {
  }
};

// 测试1: 基本入队出队
TEST_F(SuperQueueTest, BasicEnqueueDequeue)
{
  global::SuperQueue<int, 16> queue;

  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);

  EXPECT_TRUE(queue.emplace(42));
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);

  int value = 0;
  EXPECT_TRUE(queue.pop(value));
  EXPECT_EQ(value, 42);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
}

// 测试2: 队列满测试
TEST_F(SuperQueueTest, QueueFull)
{
  global::SuperQueue<int, 16> queue;

  for (int i = 0; i < 16; ++i)
  {
    EXPECT_TRUE(queue.emplace(i)) << "入队失败 at " << i;
  }

  EXPECT_EQ(queue.size(), 16);

  EXPECT_FALSE(queue.emplace(999)) << "队列已满,入队应该返回false";

  int value = 0;
  EXPECT_TRUE(queue.pop(value));
  EXPECT_EQ(value, 0);

  EXPECT_TRUE(queue.emplace(1000));
  EXPECT_EQ(queue.size(), 16);
}

// 测试3: 队列空测试
TEST_F(SuperQueueTest, QueueEmpty)
{
  global::SuperQueue<int, 16> queue;

  EXPECT_TRUE(queue.empty());

  int value = 0;
  EXPECT_FALSE(queue.pop(value)) << "队列为空,出队应该返回false";

  EXPECT_TRUE(queue.emplace(123));
  EXPECT_FALSE(queue.empty());
  EXPECT_TRUE(queue.pop(value));
  EXPECT_EQ(value, 123);

  EXPECT_FALSE(queue.pop(value));
  EXPECT_TRUE(queue.empty());
}

// 测试4: 复杂类型测试
TEST_F(SuperQueueTest, ComplexType)
{
  global::SuperQueue<TestMessage, 32> queue;

  EXPECT_TRUE(queue.emplace(1, "Hello"));
  EXPECT_TRUE(queue.emplace(2, "World"));

  EXPECT_EQ(queue.size(), 2);

  TestMessage msg;
  EXPECT_TRUE(queue.pop(msg));
  EXPECT_EQ(msg.id, 1);
  EXPECT_EQ(msg.content, "Hello");

  EXPECT_TRUE(queue.pop(msg));
  EXPECT_EQ(msg.id, 2);
  EXPECT_EQ(msg.content, "World");

  EXPECT_TRUE(queue.empty());
}

// 测试5: FIFO顺序测试
TEST_F(SuperQueueTest, FIFOOrder)
{
  global::SuperQueue<int, 128> queue;

  for (int i = 0; i < 100; ++i)
  {
    EXPECT_TRUE(queue.emplace(i));
  }

  for (int i = 0; i < 100; ++i)
  {
    int value = 0;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, i) << "FIFO顺序错误 at " << i;
  }

  EXPECT_TRUE(queue.empty());
}

// 测试6: 析构函数测试
TEST_F(SuperQueueTest, DestructorWithElements)
{
  {
    global::SuperQueue<TestMessage, 16> queue;

    for (int i = 0; i < 10; ++i)
    {
      EXPECT_TRUE(queue.emplace(i, "Test" + std::to_string(i)));
    }

    EXPECT_EQ(queue.size(), 10);

    TestMessage msg;
    for (int i = 0; i < 5; ++i)
    {
      EXPECT_TRUE(queue.pop(msg));
    }

    EXPECT_EQ(queue.size(), 5);
  }

  // 如果析构有问题,会触发崩溃
  SUCCEED();
}

// 测试7: 多线程生产者消费者测试
TEST_F(SuperQueueTest, MultithreadProducerConsumer)
{
  global::SuperQueue<int, 1024> queue;
  const int num_producers = 4;
  const int num_consumers = 4;
  const int items_per_producer = 1000;

  std::atomic<int> produced{0};
  std::atomic<int> consumed{0};
  std::vector<std::thread> threads;

  threads.reserve(num_producers);
  for (int i = 0; i < num_producers; ++i)
  {
    threads.emplace_back(
        [&queue, &produced, i]()
        {
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

  for (int i = 0; i < num_consumers; ++i)
  {
    threads.emplace_back(
        [&queue, &consumed, total = num_producers * items_per_producer]()
        {
          int value = 0;
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

  for (auto& thread : threads)
  {
    thread.join();
  }

  EXPECT_EQ(produced.load(), num_producers * items_per_producer);
  EXPECT_EQ(consumed.load(), num_producers * items_per_producer);
  EXPECT_TRUE(queue.empty());
}

// 测试8: 并发压力测试
TEST_F(SuperQueueTest, ConcurrentStressTest)
{
  global::SuperQueue<int, 512> queue;
  const int num_threads = 8;
  const int operations = 10000;
  std::atomic<int> success_count{0};

  std::vector<std::thread> threads;

  threads.reserve(num_threads);
  for (int i = 0; i < num_threads; ++i)
  {
    threads.emplace_back(
        [&queue, &success_count, i]()
        {
          for (int j = 0; j < operations; ++j)
          {
            if (j % 2 == 0)
            {
              if (queue.emplace((i * operations) + j))
              {
                success_count.fetch_add(1, std::memory_order_relaxed);
              }
            }
            else
            {
              int value = 0;
              if (queue.pop(value))
              {
                success_count.fetch_add(1, std::memory_order_relaxed);
              }
            }
          }
        });
  }

  for (auto& thread : threads)
  {
    thread.join();
  }

  int value = 0;
  while (queue.pop(value))
  {
    success_count.fetch_add(1, std::memory_order_relaxed);
  }

  EXPECT_GT(success_count.load(), 0);
}

// 测试9: size()方法准确性测试
TEST_F(SuperQueueTest, SizeAccuracy)
{
  global::SuperQueue<int, 32> queue;

  for (int i = 0; i < 20; ++i)
  {
    EXPECT_EQ(queue.size(), i);
    EXPECT_TRUE(queue.emplace(i));
    EXPECT_EQ(queue.size(), i + 1);
  }

  for (int i = 20; i > 0; --i)
  {
    EXPECT_EQ(queue.size(), i);
    int value = 0;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(queue.size(), i - 1);
  }

  EXPECT_EQ(queue.size(), 0);
  EXPECT_TRUE(queue.empty());
}

// 测试10: 快速入队出队循环测试
TEST_F(SuperQueueTest, RapidCycling)
{
  global::SuperQueue<int, 64> queue;

  for (int cycle = 0; cycle < 1000; ++cycle)
  {
    for (int i = 0; i < 50; ++i)
    {
      EXPECT_TRUE(queue.emplace((cycle * 50) + i));
    }

    for (int i = 0; i < 50; ++i)
    {
      int value = 0;
      EXPECT_TRUE(queue.pop(value));
      EXPECT_EQ(value, (cycle * 50) + i);
    }

    EXPECT_TRUE(queue.empty());
  }
}
