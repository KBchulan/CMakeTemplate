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
    const auto &logger = tools::Logger::getInstance();
    logger.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  void TearDown() override
  {
    const auto &logger = tools::Logger::getInstance();
    logger.flush();
  }
};

// 测试1: 日志打印
TEST_F(LoggerTest, BasicLogging)
{
  const auto &logger = tools::Logger::getInstance();
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
  const auto &logger = tools::Logger::getInstance();
  auto style = fmt::fg(fmt::color::cyan);
  EXPECT_NO_THROW(logger.print(style, "样式化输出: {}", "cyan"));
  EXPECT_NO_THROW(logger.print("普通打印: {}", "normal"));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

// 测试3: flush刷新
TEST_F(LoggerTest, QueueSizeTracking)
{
  const auto &logger = tools::Logger::getInstance();
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
  const auto &logger = tools::Logger::getInstance();
  EXPECT_NO_THROW(logger.info("中文日志"));
  EXPECT_NO_THROW(logger.warning("特殊符号: !@#$%^&*()"));
  EXPECT_NO_THROW(logger.error("换行\n制表\t"));

  logger.flush();
}

// 测试5: 长信息处理
TEST_F(LoggerTest, LargeMessageTruncation)
{
  const auto &logger = tools::Logger::getInstance();
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
  const auto &logger = tools::Logger::getInstance();
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
  const auto &logger = tools::Logger::getInstance();
  const int num_threads = 4;
  const int messages_per_thread = 10;
  std::atomic<int> completed_threads{0};

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  // 创建多个线程同时写日志
  for (int thr = 0; thr < num_threads; ++thr)
  {
    threads.emplace_back(
        [thr, &logger, &completed_threads]() -> void
        {
          for (int i = 0; i < messages_per_thread; ++i)
          {
            logger.info("线程{} 消息{}", thr, i);
          }
          completed_threads.fetch_add(1);
        });
  }

  for (auto &thread : threads)
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
  const auto &logger = tools::Logger::getInstance();
  const int num_threads = 3;
  std::atomic<int> completed{0};

  std::vector<std::thread> threads;

  // 每个线程使用不同的日志级别
  threads.emplace_back(
      [&completed, &logger]() -> void
      {
        for (int i = 0; i < 5; ++i)
        {
          logger.info("Info线程 {}", i);
        }
        completed.fetch_add(1);
      });

  threads.emplace_back(
      [&completed, &logger]() -> void
      {
        for (int i = 0; i < 5; ++i)
        {
          logger.warning("Warning线程 {}", i);
        }
        completed.fetch_add(1);
      });

  threads.emplace_back(
      [&completed, &logger]() -> void
      {
        for (int i = 0; i < 5; ++i)
        {
          logger.error("Error线程 {}", i);
        }
        completed.fetch_add(1);
      });

  for (auto &thr : threads)
  {
    thr.join();
  }

  EXPECT_EQ(completed.load(), num_threads);
  logger.flush();
}

// 测试9: 单例
TEST_F(LoggerTest, SingletonBehavior)
{
  const auto &logger1 = tools::Logger::getInstance();
  const auto &logger2 = tools::Logger::getInstance();

  EXPECT_EQ(&logger1, &logger2);
}

// 测试10: 稳定性测试
TEST_F(LoggerTest, QueueOperationsStability)
{
  const auto &logger = tools::Logger::getInstance();
  for (int cycle = 0; cycle < 3; ++cycle)
  {
    for (int i = 0; i < 20; ++i)
    {
      logger.info("稳定性测试 C{} M{}", cycle, i);
    }

    logger.flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(logger.queueSize(), 0);
  }
}
