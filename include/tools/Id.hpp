/******************************************************************************
 *
 * @file       Id.hpp
 * @brief      提供 唯一ID 生成工具，包含 uuid 和雪花算法
 *
 * @author     KBchulan
 * @date       2025/09/06
 * @history
 ******************************************************************************/

#ifndef ID_HPP
#define ID_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <thread>

namespace tools
{

// UUID生成器
class UuidGenerator
{
public:
  static UuidGenerator& getInstance()
  {
    static UuidGenerator instance;
    return instance;
  }

  // 生成UUID v4
  std::string generateUuid() noexcept
  {
    std::lock_guard<std::mutex> lock(_mutex);

    std::uniform_int_distribution<std::uint32_t> dis(0, 0xFFFFFFFF);
    std::uniform_int_distribution<std::uint16_t> dis16(0, 0xFFFF);

    std::uint32_t time_low = dis(_gen);
    std::uint16_t time_mid = dis16(_gen);
    std::uint16_t time_hi_and_version = (dis16(_gen) & 0x0FFF) | 0x4000;
    std::uint16_t clock_seq_and_reserved = (dis16(_gen) & 0x3FFF) | 0x8000;

    std::uint64_t node = (static_cast<std::uint64_t>(dis16(_gen)) << 32) | dis(_gen);
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

    return {result.data()};
  }

  // 生成简短UUID
  std::string generateShortUuid() noexcept
  {
    auto uuid = generateUuid();
    std::erase(uuid, '-');
    return uuid;
  }

private:
  std::mt19937 _gen;
  std::mutex _mutex;

  UuidGenerator()
  {
    std::random_device random_device;
    _gen.seed(random_device());
  }
};

// 雪花算法ID生成器
class SnowflakeIdGenerator
{
public:
  static SnowflakeIdGenerator& getInstance()
  {
    static SnowflakeIdGenerator instance;
    return instance;
  }

  // 生成雪花ID
  std::uint64_t generateId() noexcept
  {
    std::lock_guard<std::mutex> lock(_mutex);

    auto timestamp = _getCurrentTimestamp();

    // 时钟回拨检测
    if (timestamp < _last_timestamp)
    {
      while (timestamp <= _last_timestamp)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        timestamp = _getCurrentTimestamp();
      }
    }

    if (_last_timestamp == timestamp)
    {
      _sequence = (_sequence + 1) & 0xFFF;
      if (_sequence == 0)
      {
        timestamp = _waitNextMillis(_last_timestamp);
      }
    }
    else
    {
      _sequence = 0;
    }

    _last_timestamp = timestamp;

    // 组装雪花ID: 1位符号位(0) + 41位时间戳 + 10位机器ID + 12位序列号 = 64位
    return ((timestamp - EPOCH) << 22) | (static_cast<std::uint64_t>(_machine_id) << 12) | _sequence;
  }

  // 获取字符串格式的雪花ID
  std::string generateIdString() noexcept
  {
    return std::to_string(generateId());
  }

private:
  static constexpr std::uint64_t EPOCH = 1704067200000ULL;  // 2024-01-01 00:00:00 UTC

  std::uint16_t _machine_id;
  std::uint64_t _last_timestamp{0};
  std::uint16_t _sequence{0};
  std::mutex _mutex;

  SnowflakeIdGenerator() : _machine_id(_generateMachineId())
  {
    _machine_id &= 0x3FF;
  }

  [[nodiscard]] static std::uint16_t _generateMachineId() noexcept
  {
    // 使用线程ID和进程时间生成简单的机器ID
    auto thread_id = std::this_thread::get_id();
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();

    std::hash<std::thread::id> hasher;
    std::uint64_t hash_value = hasher(thread_id) ^ static_cast<std::uint64_t>(now);

    return static_cast<std::uint16_t>(hash_value & 0x3FF);
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

#define uuidGenerator tools::UuidGenerator::getInstance()
#define snowflakeGenerator tools::SnowflakeIdGenerator::getInstance()

#endif  // ID_HPP