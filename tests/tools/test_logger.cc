/******************************************************************************
 *
 * @file       test_logger.cc
 * @brief      Logger组件单元测试
 *
 * @author     KBchulan
 * @date       2025/09/05
 * @history    Logger异步日志系统单元测试套件
 ******************************************************************************/

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <tools/Logger.hpp>
#include <vector>

class LoggerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    logger.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  void TearDown() override
  {
    logger.flush();
  }
};

// 测试1: 日志打印
TEST_F(LoggerTest, BasicLogging)
{
  EXPECT_NO_THROW(logger.info("测试信息日志: {}", 42));
  EXPECT_NO_THROW(logger.warning("测试警告日志: {}", "warning"));
  EXPECT_NO_THROW(logger.error("测试错误日志: {}", 3.14));
  EXPECT_NO_THROW(logger.debug("测试调试日志: {}", true));
  EXPECT_NO_THROW(logger.trace("测试跟踪日志: {}", "trace"));
  EXPECT_NO_THROW(logger.fatal("测试致命日志: {}", "fatal"));

  // 等待异步处理完成
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// 测试2: 普通打印
TEST_F(LoggerTest, PrintWithStyle)
{
  auto style = fmt::fg(fmt::color::cyan);
  EXPECT_NO_THROW(logger.print(style, "样式化输出: {}", "cyan"));
  EXPECT_NO_THROW(logger.print("普通打印: {}", "normal"));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

// 测试3: flush刷新
TEST_F(LoggerTest, QueueSizeTracking)
{
  for (int i = 0; i < 50; ++i)
  {
    logger.info("消息 {}", i);
  }

  EXPECT_GE(logger.queueSize(), 0);

  logger.flush();
  EXPECT_EQ(logger.queueSize(), 0);
}

// 测试4: 不同符号测试
TEST_F(LoggerTest, SpecialCharacters)
{
  EXPECT_NO_THROW(logger.info("中文日志"));
  EXPECT_NO_THROW(logger.warning("特殊符号: !@#$%^&*()"));
  EXPECT_NO_THROW(logger.error("换行\n制表\t"));

  logger.flush();
}

// 测试5: 长信息处理
TEST_F(LoggerTest, LargeMessageTruncation)
{
  std::string long_message(1000, 'A');

  // 内部会截断到MAX_MESSAGE_SIZE-1
  EXPECT_NO_THROW(logger.info("长消息测试: {}", long_message));

  // 测试边界情况
  std::string boundary_message(500, 'B');
  EXPECT_NO_THROW(logger.info("边界消息: {}", boundary_message));

  // 测试空消息
  EXPECT_NO_THROW(logger.info(""));

  logger.flush();
}

// 测试6: 各种类型数据输出
TEST_F(LoggerTest, VariousFormatTypes)
{
  EXPECT_NO_THROW(logger.info("整数: {}", 42));
  EXPECT_NO_THROW(logger.info("浮点: {:.2f}", 3.14159));
  EXPECT_NO_THROW(logger.info("字符串: {}", "hello"));
  EXPECT_NO_THROW(logger.info("布尔: {}", true));
  EXPECT_NO_THROW(logger.info("多参数: {} {} {}", 1, 2.0, "three"));

  logger.flush();
}

// 测试7: 多线程安全测试
TEST_F(LoggerTest, MultithreadSafety)
{
  const int num_threads = 4;
  const int messages_per_thread = 10;
  std::atomic<int> completed_threads{0};

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  // 创建多个线程同时写日志
  for (int thr = 0; thr < num_threads; ++thr)
  {
    threads.emplace_back(
        [thr, &completed_threads]() -> void
        {
          for (int i = 0; i < messages_per_thread; ++i)
          {
            logger.info("线程{} 消息{}", thr, i);
          }
          completed_threads.fetch_add(1);
        });
  }

  for (auto& thread : threads)
  {
    thread.join();
  }

  EXPECT_EQ(completed_threads.load(), num_threads);

  logger.flush();
  EXPECT_EQ(logger.queueSize(), 0);
}

// 测试8: 测试不同日志类型
TEST_F(LoggerTest, ConcurrentMixedLevels)
{
  const int num_threads = 3;
  std::atomic<int> completed{0};

  std::vector<std::thread> threads;

  // 每个线程使用不同的日志级别
  threads.emplace_back(
      [&completed]() -> void
      {
        for (int i = 0; i < 5; ++i)
        {
          logger.info("Info线程 {}", i);
        }
        completed.fetch_add(1);
      });

  threads.emplace_back(
      [&completed]() -> void
      {
        for (int i = 0; i < 5; ++i)
        {
          logger.warning("Warning线程 {}", i);
        }
        completed.fetch_add(1);
      });

  threads.emplace_back(
      [&completed]() -> void
      {
        for (int i = 0; i < 5; ++i)
        {
          logger.error("Error线程 {}", i);
        }
        completed.fetch_add(1);
      });

  for (auto& thr : threads)
  {
    thr.join();
  }

  EXPECT_EQ(completed.load(), num_threads);
  logger.flush();
}

// 测试9: 单例
TEST_F(LoggerTest, SingletonBehavior)
{
  auto& logger1 = tools::Logger::getInstance();
  auto& logger2 = tools::Logger::getInstance();

  EXPECT_EQ(&logger1, &logger2);
}

// 测试10: 稳定性测试
TEST_F(LoggerTest, QueueOperationsStability)
{
  for (int cycle = 0; cycle < 3; ++cycle)
  {
    for (int i = 0; i < 20; ++i)
    {
      logger.info("稳定性测试 C{} M{}", cycle, i);
    }

    logger.flush();
    EXPECT_EQ(logger.queueSize(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

// 测试11: 极限性能测试
TEST_F(LoggerTest, ExtremeThroughputTest)
{
  const int message_count = 1000000;
  const int num_threads = 8;
  const int messages_per_thread = message_count / num_threads;

  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  std::atomic<int> completed_threads{0};

  auto start = std::chrono::high_resolution_clock::now();

  // 启动多个线程同时投递消息
  for (int thr = 0; thr < num_threads; ++thr)
  {
    threads.emplace_back(
        [thr, &completed_threads]()
        {
          int start_idx = thr * messages_per_thread;
          for (int i = 0; i < messages_per_thread; ++i)
          {
            int msg_id = start_idx + i;
            logger.info("极限测试T{} 消息#{} 数据{}", thr, msg_id, msg_id * 42);
          }
          completed_threads.fetch_add(1);
        });
  }

  for (auto& thread : threads)
  {
    thread.join();
  }

  logger.flush();
  auto flush_end = std::chrono::high_resolution_clock::now();

  EXPECT_EQ(completed_threads.load(), num_threads);

  auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(flush_end - start);
  double total_time_sec = static_cast<double>(total_duration.count()) / 1000000.0;
  double total_throughput = static_cast<double>(message_count) / total_time_sec;

  EXPECT_GT(total_throughput, 3000000.0);

  EXPECT_EQ(logger.queueSize(), 0) << "测试结束后队列未完全清空";
}