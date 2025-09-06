/******************************************************************************
 *
 * @file       test_id.cc
 * @brief      ID生成器组件单元测试
 *
 * @author     KBchulan
 * @date       2025/09/06
 * @history    UUID和雪花算法ID生成器单元测试套件
 ******************************************************************************/

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <set>
#include <string>
#include <thread>
#include <tools/Id.hpp>
#include <unordered_set>
#include <vector>

class IdTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  void TearDown() override
  {
  }
};

// 测试1: UUID基本生成
TEST_F(IdTest, UuidBasicGeneration)
{
  EXPECT_NO_THROW(uuidGenerator.generateUuid());
  EXPECT_NO_THROW(uuidGenerator.generateShortUuid());

  auto uuid = uuidGenerator.generateUuid();
  auto shortUuid = uuidGenerator.generateShortUuid();

  // UUID长度检查
  EXPECT_EQ(uuid.length(), 36);
  EXPECT_EQ(shortUuid.length(), 32);

  // UUID格式检查
  EXPECT_EQ(uuid[8], '-');
  EXPECT_EQ(uuid[13], '-');
  EXPECT_EQ(uuid[18], '-');
  EXPECT_EQ(uuid[23], '-');

  // short UUID不应包含连字符
  EXPECT_EQ(shortUuid.find('-'), std::string::npos);
}

// 测试2: UUID唯一性
TEST_F(IdTest, UuidUniqueness)
{
  const int count = 1000;
  std::unordered_set<std::string> uuid_set;
  std::unordered_set<std::string> short_uuid_set;

  for (int i = 0; i < count; ++i)
  {
    auto uuid = uuidGenerator.generateUuid();
    auto shortUuid = uuidGenerator.generateShortUuid();

    // 检查唯一性
    EXPECT_TRUE(uuid_set.insert(uuid).second) << "UUID重复: " << uuid;
    EXPECT_TRUE(short_uuid_set.insert(shortUuid).second) << "Short UUID重复: " << shortUuid;
  }

  EXPECT_EQ(uuid_set.size(), count);
  EXPECT_EQ(short_uuid_set.size(), count);
}

// 测试3: 雪花ID基本生成
TEST_F(IdTest, SnowflakeBasicGeneration)
{
  EXPECT_NO_THROW(snowflakeGenerator.generateId());
  EXPECT_NO_THROW(snowflakeGenerator.generateIdString());

  auto sid = snowflakeGenerator.generateId();
  auto idStr = snowflakeGenerator.generateIdString();

  EXPECT_GT(sid, 0);
  EXPECT_FALSE(idStr.empty());
}

// 测试4: 雪花ID唯一性
TEST_F(IdTest, SnowflakeUniqueness)
{
  const int count = 10000;
  std::set<std::uint64_t> id_set;

  for (int i = 0; i < count; ++i)
  {
    auto sid = snowflakeGenerator.generateId();
    EXPECT_TRUE(id_set.insert(sid).second) << "雪花ID重复: " << sid;
  }

  EXPECT_EQ(id_set.size(), count);
}

// 测试5: 雪花ID递增性
TEST_F(IdTest, SnowflakeMonotonic)
{
  const int count = 100;
  std::uint64_t prev_id = 0;

  for (int i = 0; i < count; ++i)
  {
    auto sid = snowflakeGenerator.generateId();
    EXPECT_GT(sid, prev_id) << "雪花ID不是递增的: " << prev_id << " >= " << sid;
    prev_id = sid;
  }
}

// 测试6: UUID版本检查
TEST_F(IdTest, UuidVersionCheck)
{
  auto uuid = uuidGenerator.generateUuid();

  EXPECT_EQ(uuid[14], '4');

  char variant = uuid[19];
  EXPECT_TRUE(variant == '8' || variant == '9' || variant == 'a' || variant == 'b');
}

// 测试7: 特殊字符测试
TEST_F(IdTest, SpecialCharacters)
{
  // UUID只应包含十六进制字符和连字符
  auto uuid = uuidGenerator.generateUuid();
  auto shortUuid = uuidGenerator.generateShortUuid();

  for (char cha : uuid)
  {
    EXPECT_TRUE((cha >= '0' && cha <= '9') || (cha >= 'a' && cha <= 'f') || cha == '-') << "UUID包含非法字符: " << cha;
  }

  for (char cha : shortUuid)
  {
    EXPECT_TRUE((cha >= '0' && cha <= '9') || (cha >= 'a' && cha <= 'f')) << "Short UUID包含非法字符: " << cha;
  }
}

// 测试8: 多线程安全测试
TEST_F(IdTest, MultithreadSafety)
{
  const int num_threads = 8;
  const int ids_per_thread = 1000;
  std::atomic<int> completed_threads{0};

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  std::mutex uuid_mutex;
  std::mutex snowflake_mutex;
  std::unordered_set<std::string> uuid_set;
  std::set<std::uint64_t> snowflake_set;

  // 创建多个线程同时生成ID
  for (int thr = 0; thr < num_threads; ++thr)
  {
    threads.emplace_back(
        [&completed_threads, &uuid_mutex, &snowflake_mutex, &uuid_set, &snowflake_set]() -> void
        {
          for (int i = 0; i < ids_per_thread; ++i)
          {
            auto uuid = uuidGenerator.generateUuid();
            {
              std::lock_guard<std::mutex> lock(uuid_mutex);
              EXPECT_TRUE(uuid_set.insert(uuid).second) << "多线程UUID重复: " << uuid;
            }

            auto snowflake = snowflakeGenerator.generateId();
            {
              std::lock_guard<std::mutex> lock(snowflake_mutex);
              EXPECT_TRUE(snowflake_set.insert(snowflake).second) << "多线程雪花ID重复: " << snowflake;
            }
          }
          completed_threads.fetch_add(1);
        });
  }

  for (auto& thread : threads)
  {
    thread.join();
  }

  EXPECT_EQ(completed_threads.load(), num_threads);
  EXPECT_EQ(uuid_set.size(), num_threads * ids_per_thread);
  EXPECT_EQ(snowflake_set.size(), num_threads * ids_per_thread);
}

// 测试9: 单例行为测试
TEST_F(IdTest, SingletonBehavior)
{
  auto& uuid1 = tools::UuidGenerator::getInstance();
  auto& uuid2 = tools::UuidGenerator::getInstance();
  auto& snowflake1 = tools::SnowflakeIdGenerator::getInstance();
  auto& snowflake2 = tools::SnowflakeIdGenerator::getInstance();

  EXPECT_EQ(&uuid1, &uuid2);
  EXPECT_EQ(&snowflake1, &snowflake2);
}

// 测试10: 性能基准测试
TEST_F(IdTest, PerformanceBenchmark)
{
  const int burst_count = 10000;
  const int uuid_count = burst_count / 10;

  auto start = std::chrono::high_resolution_clock::now();

  // 雪花ID
  for (int i = 0; i < burst_count; ++i)
  {
    auto sid = snowflakeGenerator.generateId();
    EXPECT_GT(sid, 0);
  }

  auto mid = std::chrono::high_resolution_clock::now();

  // 生成UUID
  for (int i = 0; i < uuid_count; ++i)
  {
    auto uuid = uuidGenerator.generateUuid();
    EXPECT_EQ(uuid.length(), 36);
  }

  auto end = std::chrono::high_resolution_clock::now();

  auto snowflake_time = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
  auto uuid_time = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);

  // 性能检查 - 雪花ID应该很快
  double snowflake_rate = static_cast<double>(burst_count) / (static_cast<double>(snowflake_time.count()) / 1000000.0);
  EXPECT_GT(snowflake_rate, 50000.0) << "雪花ID生成速度过慢: " << snowflake_rate << " ids/sec";

  // 性能检查 - UUID虽然慢但不能太慢
  double uuid_rate = static_cast<double>(uuid_count) / (static_cast<double>(uuid_time.count()) / 1000000.0);
  EXPECT_GT(uuid_rate, 1000.0) << "UUID生成速度过慢: " << uuid_rate << " ids/sec";
}

// 测试11: 并发混合生成测试
TEST_F(IdTest, ConcurrentMixedGeneration)
{
  const int num_threads = 4;
  const int operations = 500;
  std::atomic<int> completed{0};

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  std::mutex result_mutex;
  std::unordered_set<std::string> all_uuids;
  std::set<std::uint64_t> all_snowflakes;

  for (int thr = 0; thr < num_threads; ++thr)
  {
    threads.emplace_back(
        [&completed, &result_mutex, &all_uuids, &all_snowflakes]() -> void
        {
          std::unordered_set<std::string> local_uuids;
          std::set<std::uint64_t> local_snowflakes;

          for (int i = 0; i < operations; ++i)
          {
            // 混合生成UUID和雪花ID
            if (i % 2 == 0)
            {
              auto uuid = uuidGenerator.generateUuid();
              local_uuids.insert(uuid);
            }
            else
            {
              auto snowflake = snowflakeGenerator.generateId();
              local_snowflakes.insert(snowflake);
            }
          }

          {
            std::lock_guard<std::mutex> lock(result_mutex);
            all_uuids.insert(local_uuids.begin(), local_uuids.end());
            all_snowflakes.insert(local_snowflakes.begin(), local_snowflakes.end());
          }

          completed.fetch_add(1);
        });
  }

  for (auto& thr : threads)
  {
    thr.join();
  }

  EXPECT_EQ(completed.load(), num_threads);

  // 验证没有重复
  size_t expected_uuids = (num_threads * operations + 1) / 2;
  size_t expected_snowflakes = num_threads * operations / 2;

  EXPECT_EQ(all_uuids.size(), expected_uuids);
  EXPECT_EQ(all_snowflakes.size(), expected_snowflakes);
}