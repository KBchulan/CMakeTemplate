/******************************************************************************
 *
 * @file       Id.hpp
 * @brief      提供 唯一ID 生成工具，包含 uuid 和雪花算法
 *
 * @author     KBchulan
 * @date       2025/09/06
 * @history    重构为thread_local实现，消除锁竞争，提升并发性能
 ******************************************************************************/

#ifndef ID_HPP
#define ID_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <expected>
#include <global/Global.hpp>
#include <random>
#include <string>
#include <thread>

namespace tools
{

// UUID生成错误类型
enum class UuidError : std::uint8_t
{
  GenerationFailed
};

// 雪花算法错误类型
enum class SnowflakeError : std::uint8_t
{
  ClockRollbackTooLarge,
  WaitTimeout,
  TimestampOverflow
};

// UUID生成器
class UuidGenerator
{
public:
  // 生成UUID v4
  static std::expected<std::string, UuidError> generateUuid() noexcept
  {
    try
    {
      static thread_local std::mt19937 generator = _initializeGenerator();

      std::uniform_int_distribution<std::uint32_t> dis(0, 0xFFFFFFFF);
      std::uniform_int_distribution<std::uint16_t> dis16(0, 0xFFFF);

      std::uint32_t time_low = dis(generator);
      std::uint16_t time_mid = dis16(generator);
      std::uint16_t time_hi_and_version = (dis16(generator) & 0x0FFF) | 0x4000;
      std::uint16_t clock_seq_and_reserved = (dis16(generator) & 0x3FFF) | 0x8000;

      std::uint64_t node = (static_cast<std::uint64_t>(dis16(generator)) << 32) | dis(generator);
      node &= 0xFFFFFFFFFFFFULL;

      static constexpr std::array<char, 16> hex_chars = {'0', '1', '2', '3', '4', '5', '6', '7',
                                                         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
      std::array<char, 37> result{};

      // Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
      std::uint32_t temp;
      char* ptr = result.data();

      // time_low (8 chars)
      temp = time_low;
      for (int i = 7; i >= 0; --i)
      {
        ptr[i] = hex_chars[temp & 0xF];
        temp >>= 4;
      }
      ptr += 8;
      *ptr++ = '-';

      // time_mid (4 chars)
      temp = time_mid;
      for (int i = 3; i >= 0; --i)
      {
        ptr[i] = hex_chars[temp & 0xF];
        temp >>= 4;
      }
      ptr += 4;
      *ptr++ = '-';

      // time_hi_and_version (4 chars)
      temp = time_hi_and_version;
      for (int i = 3; i >= 0; --i)
      {
        ptr[i] = hex_chars[temp & 0xF];
        temp >>= 4;
      }
      ptr += 4;
      *ptr++ = '-';

      // clock_seq_and_reserved (4 chars)
      temp = clock_seq_and_reserved;
      for (int i = 3; i >= 0; --i)
      {
        ptr[i] = hex_chars[temp & 0xF];
        temp >>= 4;
      }
      ptr += 4;
      *ptr++ = '-';

      // node (12 chars)
      std::uint64_t node_temp = node;
      for (int i = 11; i >= 0; --i)
      {
        ptr[i] = hex_chars[node_temp & 0xF];
        node_temp >>= 4;
      }
      ptr += 12;
      *ptr = '\0';

      return std::string{result.data()};
    }
    catch (...)
    {
      return std::unexpected(UuidError::GenerationFailed);
    }
  }

  // 生成简短UUID
  static std::expected<std::string, UuidError> generateShortUuid() noexcept
  {
    auto uuid_result = generateUuid();
    if (!uuid_result.has_value())
    {
      return std::unexpected(uuid_result.error());
    }

    auto uuid = std::move(*uuid_result);
    std::erase(uuid, '-');
    return uuid;
  }

private:
  // 随机数生成器初始化
  static std::mt19937 _initializeGenerator() noexcept
  {
    std::array<std::uint32_t, std::mt19937::state_size> seed_data;
    std::random_device random_device;

    std::ranges::generate(seed_data, std::ref(random_device));
    std::seed_seq seq(seed_data.begin(), seed_data.end());

    std::mt19937 generator;
    generator.seed(seq);
    return generator;
  }
};

// 雪花算法ID生成器
class SnowflakeIdGenerator
{
public:
  // 生成雪花ID
  static std::expected<std::uint64_t, SnowflakeError> generateId() noexcept
  {
    static thread_local ThreadState thread_state = _initializeThreadState();

    auto timestamp = _getCurrentTimestamp();

    // 时钟回拨检测和处理
    if (timestamp < thread_state.last_timestamp)
    {
      auto rollback_ms = thread_state.last_timestamp - timestamp;

      if (rollback_ms > global::snowflake::MAX_CLOCK_ROLLBACK_MS)
      {
        return std::unexpected(SnowflakeError::ClockRollbackTooLarge);
      }

      auto start_wait = std::chrono::steady_clock::now();
      auto max_wait = std::chrono::milliseconds(global::snowflake::MAX_ROLLBACK_WAIT_MS);

      while (timestamp <= thread_state.last_timestamp)
      {
        if (std::chrono::steady_clock::now() - start_wait > max_wait)
        {
          return std::unexpected(SnowflakeError::WaitTimeout);
        }

        std::this_thread::sleep_for(global::snowflake::SLEEP_INTERVAL);
        timestamp = _getCurrentTimestamp();
      }
    }

    if (thread_state.last_timestamp == timestamp)
    {
      thread_state.sequence = (thread_state.sequence + 1) & global::snowflake::SEQUENCE_MASK;
      if (thread_state.sequence == 0)
      {
        timestamp = _waitNextMillis(thread_state.last_timestamp);
      }
    }
    else
    {
      thread_state.sequence = 0;
    }

    thread_state.last_timestamp = timestamp;

    auto relative_timestamp = timestamp - global::snowflake::EPOCH;
    if (relative_timestamp >= (1ULL << global::snowflake::TIMESTAMP_MAX_BITS))
    {
      return std::unexpected(SnowflakeError::TimestampOverflow);
    }

    // 组装雪花ID: 1位符号位(0) + 41位时间戳 + 10位worker_id + 12位序列号 = 64位
    return (relative_timestamp << global::snowflake::TIMESTAMP_SHIFT) |
           (static_cast<std::uint64_t>(thread_state.worker_id) << global::snowflake::WORKER_ID_SHIFT) |
           thread_state.sequence;
  }

  // 获取字符串格式的雪花ID
  static std::expected<std::string, SnowflakeError> generateIdString() noexcept
  {
    auto id_result = generateId();
    if (!id_result)
    {
      return std::unexpected(id_result.error());
    }

    return std::to_string(*id_result);
  }

private:
  struct ThreadState
  {
    std::uint16_t worker_id{0};
    std::uint64_t last_timestamp{0};
    std::uint16_t sequence{0};
  };

  static inline std::atomic<std::uint16_t> global_worker_counter{0};

  static ThreadState _initializeThreadState() noexcept
  {
    ThreadState state;

    state.worker_id = global_worker_counter.fetch_add(1, std::memory_order_relaxed) & global::snowflake::WORKER_ID_MASK;

    return state;
  }

  [[nodiscard]] static std::uint64_t _getCurrentTimestamp() noexcept
  {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
  }

  [[nodiscard]] static std::uint64_t _waitNextMillis(std::uint64_t last_timestamp) noexcept
  {
    auto timestamp = _getCurrentTimestamp();
    while (timestamp <= last_timestamp)
    {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      timestamp = _getCurrentTimestamp();
    }
    return timestamp;
  }
};

}  // namespace tools

#endif  // ID_HPP