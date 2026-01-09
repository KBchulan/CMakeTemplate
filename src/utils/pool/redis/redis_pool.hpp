/******************************************************************************
 *
 * @file       redis_pool.hpp
 * @brief      封装的 Redis 连接池子
 *
 * @author     KBchulan
 * @date       2025/12/08
 * @history
 ******************************************************************************/

#ifndef REDIS_POOL_HPP
#define REDIS_POOL_HPP

#include <hiredis/hiredis.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <global/Global.hpp>
#include <optional>
#include <string>
#include <utils/UtilsExport.hpp>
#include <vector>

namespace utils
{

struct UTILS_EXPORT RedisConfig
{
  std::string host;
  std::uint16_t port;
  std::string password;
  std::size_t db_index;
  std::size_t pool_size;
  std::chrono::seconds timeout;
};

class UTILS_EXPORT RedisReply
{
public:
  explicit RedisReply(redisReply* reply);
  ~RedisReply();

  [[nodiscard]] redisReply* GetReply() const;
  [[nodiscard]] bool IsValid() const;
  [[nodiscard]] bool IsNil() const;
  [[nodiscard]] bool IsError() const;
  [[nodiscard]] std::optional<std::string> AsString() const;
  [[nodiscard]] std::optional<std::int64_t> AsInteger() const;
  [[nodiscard]] std::optional<std::vector<std::string>> AsArray() const;

  RedisReply(const RedisReply&) = delete;
  RedisReply& operator=(const RedisReply&) = delete;
  RedisReply(RedisReply&& other) noexcept;
  RedisReply& operator=(RedisReply&&) noexcept;

private:
  redisReply* _reply;
};

class UTILS_EXPORT PipeLine
{
public:
  explicit PipeLine(redisContext* ctx);
  PipeLine& Append(const char* format, ...);
  std::vector<RedisReply> Execute();

private:
  redisContext* _ctx;
  std::size_t _command_count;
};

class RedisPool;
class UTILS_EXPORT PooledRedisConnection
{
public:
  PooledRedisConnection(redisContext* ctx, RedisPool* pool, std::size_t slot);
  ~PooledRedisConnection();

  [[nodiscard]] redisContext* GetContext() const;

  // 基础 Key 操作
  RedisReply Set(const std::string& key, const std::string& value);
  RedisReply SetEX(const std::string& key, const std::string& value, std::size_t seconds);
  RedisReply SetNX(const std::string& key, const std::string& value);
  RedisReply Get(const std::string& key);
  RedisReply Del(const std::string& key);
  RedisReply Del(const std::vector<std::string>& keys);
  RedisReply Exists(const std::string& key);
  RedisReply Expire(const std::string& key, std::size_t seconds);
  RedisReply TTL(const std::string& key);

  // String 操作
  RedisReply Incr(const std::string& key);
  RedisReply Decr(const std::string& key);
  RedisReply IncrBy(const std::string& key, std::int64_t increment);
  RedisReply DecrBy(const std::string& key, std::int64_t decrement);

  // Hash 操作
  RedisReply HSet(const std::string& key, const std::string& field, const std::string& value);
  RedisReply HGet(const std::string& key, const std::string& field);
  RedisReply HGetAll(const std::string& key);
  RedisReply HDel(const std::string& key, const std::string& field);
  RedisReply HExists(const std::string& key, const std::string& field);
  RedisReply HIncrBy(const std::string& key, const std::string& field, std::int64_t increment);

  // List 操作
  RedisReply LPush(const std::string& key, const std::string& value);
  RedisReply RPush(const std::string& key, const std::string& value);
  RedisReply LPop(const std::string& key);
  RedisReply RPop(const std::string& key);
  RedisReply LRange(const std::string& key, std::int64_t start, std::int64_t stop);
  RedisReply LLen(const std::string& key);
  RedisReply LTrim(const std::string& key, std::int64_t start, std::int64_t stop);

  // Set 操作
  RedisReply SAdd(const std::string& key, const std::string& member);
  RedisReply SAdd(const std::string& key, const std::vector<std::string>& members);
  RedisReply SRem(const std::string& key, const std::string& member);
  RedisReply SMembers(const std::string& key);
  RedisReply SIsMember(const std::string& key, const std::string& member);
  RedisReply SCard(const std::string& key);

  // Sorted Set 操作
  RedisReply ZAdd(const std::string& key, double score, const std::string& member);
  RedisReply ZRem(const std::string& key, const std::string& member);
  RedisReply ZRange(const std::string& key, std::int64_t start, std::int64_t stop);
  RedisReply ZRevRange(const std::string& key, std::int64_t start, std::int64_t stop);
  RedisReply ZRangeByScore(const std::string& key, double min, double max);
  RedisReply ZScore(const std::string& key, const std::string& member);
  RedisReply ZCard(const std::string& key);

  // 工具方法
  PipeLine NewPipeLine();
  RedisReply Ping();

  // 自定义命令
  RedisReply Command(const char* format, ...);

  PooledRedisConnection(const PooledRedisConnection&) = delete;
  PooledRedisConnection& operator=(const PooledRedisConnection&) = delete;
  PooledRedisConnection(PooledRedisConnection&& other) noexcept;
  PooledRedisConnection& operator=(PooledRedisConnection&&) noexcept = delete;

private:
  redisContext* _ctx;
  RedisPool* _pool;
  std::size_t _slot;
};

class UTILS_EXPORT RedisPool
{
  static constexpr std::size_t MAX_POOL_SIZE = global::server::REDIS_MAX_POOL_SIZE;

  struct Slot
  {
    redisContext* _ctx = nullptr;
    std::atomic<bool> _in_use{false};
  };

public:
  static RedisPool& GetInstance();

  void Init(const RedisConfig& config);

  [[nodiscard]] PooledRedisConnection GetConnection();

  void ReleaseConnection(std::size_t slot);

private:
  RedisPool();
  ~RedisPool();

  [[nodiscard]] redisContext* create_connection() const;

  RedisConfig _config;
  std::size_t _pool_size;
  std::array<Slot, MAX_POOL_SIZE> _slots;
  std::atomic<std::size_t> _available;
};

}  // namespace utils

#endif  // REDIS_POOL_HPP