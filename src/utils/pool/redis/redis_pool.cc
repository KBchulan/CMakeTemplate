#include "redis_pool.hpp"

#include <atomic>
#include <cstdarg>
#include <tools/Logger.hpp>

namespace utils
{

// ============================================================================
// RedisReply 实现
// ============================================================================

RedisReply::RedisReply(redisReply* reply) : _reply(reply)
{
}

RedisReply::~RedisReply()
{
  if (_reply != nullptr)
  {
    freeReplyObject(_reply);
  }
}

RedisReply::RedisReply(RedisReply&& other) noexcept : _reply(other._reply)
{
  other._reply = nullptr;
}

RedisReply& RedisReply::operator=(RedisReply&& other) noexcept
{
  if (this != &other)
  {
    if (_reply != nullptr)
    {
      freeReplyObject(_reply);
    }
    _reply = other._reply;
    other._reply = nullptr;
  }
  return *this;
}

redisReply* RedisReply::GetReply() const
{
  return _reply;
}

bool RedisReply::IsValid() const
{
  return _reply != nullptr;
}

bool RedisReply::IsNil() const
{
  return _reply != nullptr && _reply->type == REDIS_REPLY_NIL;
}

bool RedisReply::IsError() const
{
  return _reply != nullptr && _reply->type == REDIS_REPLY_ERROR;
}

std::optional<std::string> RedisReply::AsString() const
{
  if (_reply == nullptr)
  {
    return std::nullopt;
  }

  if (_reply->type == REDIS_REPLY_STRING || _reply->type == REDIS_REPLY_STATUS)
  {
    return std::string(_reply->str, _reply->len);
  }

  return std::nullopt;
}

std::optional<std::int64_t> RedisReply::AsInteger() const
{
  if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER)
  {
    return std::nullopt;
  }

  return _reply->integer;
}

std::optional<std::vector<std::string>> RedisReply::AsArray() const
{
  if (_reply == nullptr || _reply->type != REDIS_REPLY_ARRAY)
  {
    return std::nullopt;
  }
  std::vector<std::string> result;
  result.reserve(_reply->elements);

  for (std::size_t i = 0; i < _reply->elements; ++i)
  {
    redisReply* elem = _reply->element[i];
    if (elem != nullptr && (elem->type == REDIS_REPLY_STRING || elem->type == REDIS_REPLY_STATUS))
    {
      result.emplace_back(elem->str, elem->len);
    }
    else
    {
      result.emplace_back();
    }
  }
  return result;
}

// ============================================================================
// PipeLine 实现
// ============================================================================
PipeLine::PipeLine(redisContext* ctx) : _ctx(ctx), _command_count(0)
{
}

PipeLine& PipeLine::Append(const char* format, ...)
{
  va_list app;
  va_start(app, format);
  redisvAppendCommand(_ctx, format, app);
  va_end(app);
  ++_command_count;
  return *this;
}

std::vector<RedisReply> PipeLine::Execute()
{
  std::vector<RedisReply> results;
  results.reserve(_command_count);

  for (std::size_t i = 0; i < _command_count; ++i)
  {
    redisReply* reply = nullptr;
    if (redisGetReply(_ctx, reinterpret_cast<void**>(&reply)) != REDIS_OK)
    {
      results.emplace_back(nullptr);
    }
    else
    {
      results.emplace_back(reply);
    }
  }

  _command_count = 0;
  return results;
}

// ============================================================================
// PooledRedisConnection 实现
// ============================================================================

PooledRedisConnection::PooledRedisConnection(redisContext* ctx, RedisPool* pool, std::size_t slot)
    : _ctx(ctx), _pool(pool), _slot(slot)
{
}

PooledRedisConnection::~PooledRedisConnection()
{
  if (_ctx != nullptr && _pool != nullptr)
  {
    _pool->ReleaseConnection(_slot);
  }
}

PooledRedisConnection::PooledRedisConnection(PooledRedisConnection&& other) noexcept
    : _ctx(other._ctx), _pool(other._pool), _slot(other._slot)
{
  other._ctx = nullptr;
  other._pool = nullptr;
}

redisContext* PooledRedisConnection::GetContext() const
{
  return _ctx;
}

RedisReply PooledRedisConnection::Set(const std::string& key, const std::string& value)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "SET %b %b", key.data(), key.size(), value.data(), value.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::SetEX(const std::string& key, const std::string& value, std::size_t seconds)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "SETEX %b %lld %b", key.data(), key.size(),
                                                      static_cast<long long>(seconds), value.data(), value.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::SetNX(const std::string& key, const std::string& value)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "SETNX %b %b", key.data(), key.size(), value.data(), value.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::Get(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "GET %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::Del(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "DEL %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::Del(const std::vector<std::string>& keys)
{
  if (keys.empty())
  {
    return RedisReply(nullptr);
  }

  std::vector<const char*> argv;
  std::vector<std::size_t> argvlen;

  argv.push_back("DEL");
  argvlen.push_back(3);
  int count = 1;

  for (const auto& key : keys)
  {
    argv.push_back(key.data());
    argvlen.push_back(key.size());
    count++;
  }

  auto* reply = static_cast<redisReply*>(redisCommandArgv(_ctx, count, argv.data(), argvlen.data()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::Exists(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "EXISTS %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::Expire(const std::string& key, std::size_t seconds)
{
  auto* reply = static_cast<redisReply*>(
      redisCommand(_ctx, "EXPIRE %b %lld", key.data(), key.size(), static_cast<long long>(seconds)));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::TTL(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "TTL %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::Incr(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "INCR %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::Decr(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "DECR %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::IncrBy(const std::string& key, std::int64_t increment)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "INCRBY %b %lld", key.data(), key.size(), increment));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::DecrBy(const std::string& key, std::int64_t decrement)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "DECRBY %b %lld", key.data(), key.size(), decrement));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::HSet(const std::string& key, const std::string& field, const std::string& value)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "HSET %b %b %b", key.data(), key.size(), field.data(),
                                                      field.size(), value.data(), value.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::HGet(const std::string& key, const std::string& field)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "HGET %b %b", key.data(), key.size(), field.data(), field.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::HGetAll(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "HGETALL %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::HDel(const std::string& key, const std::string& field)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "HDEL %b %b", key.data(), key.size(), field.data(), field.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::HExists(const std::string& key, const std::string& field)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "HEXISTS %b %b", key.data(), key.size(), field.data(), field.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::HIncrBy(const std::string& key, const std::string& field, std::int64_t increment)
{
  auto* reply = static_cast<redisReply*>(
      redisCommand(_ctx, "HINCRBY %b %b %lld", key.data(), key.size(), field.data(), field.size(), increment));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::LPush(const std::string& key, const std::string& value)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "LPUSH %b %b", key.data(), key.size(), value.data(), value.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::RPush(const std::string& key, const std::string& value)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "RPUSH %b %b", key.data(), key.size(), value.data(), value.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::LPop(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "LPOP %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::RPop(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "RPOP %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::LRange(const std::string& key, std::int64_t start, std::int64_t stop)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "LRANGE %b %lld %lld", key.data(), key.size(), start, stop));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::LLen(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "LLEN %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::LTrim(const std::string& key, std::int64_t start, std::int64_t stop)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "LTRIM %b %lld %lld", key.data(), key.size(), start, stop));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::SAdd(const std::string& key, const std::string& member)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "SADD %b %b", key.data(), key.size(), member.data(), member.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::SAdd(const std::string& key, const std::vector<std::string>& members)
{
  if (members.empty())
  {
    return RedisReply(nullptr);
  }

  std::vector<const char*> argv;
  std::vector<std::size_t> argvlen;

  argv.push_back("SADD");
  argvlen.push_back(4);

  argv.push_back(key.data());
  argvlen.push_back(key.size());
  int count = 2;

  for (const auto& member : members)
  {
    argv.push_back(member.data());
    argvlen.push_back(member.size());
    count++;
  }

  auto* reply = static_cast<redisReply*>(redisCommandArgv(_ctx, count, argv.data(), argvlen.data()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::SRem(const std::string& key, const std::string& member)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "SREM %b %b", key.data(), key.size(), member.data(), member.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::SMembers(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "SMEMBERS %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::SIsMember(const std::string& key, const std::string& member)
{
  auto* reply = static_cast<redisReply*>(
      redisCommand(_ctx, "SISMEMBER %b %b", key.data(), key.size(), member.data(), member.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::SCard(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "SCARD %b", key.data(), key.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::ZAdd(const std::string& key, double score, const std::string& member)
{
  auto score_str = std::to_string(score);
  auto* reply = static_cast<redisReply*>(
      redisCommand(_ctx, "ZADD %b %s %b", key.data(), key.size(), score_str.c_str(), member.data(), member.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::ZRem(const std::string& key, const std::string& member)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "ZREM %b %b", key.data(), key.size(), member.data(), member.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::ZRange(const std::string& key, std::int64_t start, std::int64_t stop)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "ZRANGE %b %lld %lld", key.data(), key.size(), start, stop));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::ZRevRange(const std::string& key, std::int64_t start, std::int64_t stop)
{
  auto* reply =
      static_cast<redisReply*>(redisCommand(_ctx, "ZREVRANGE %b %lld %lld", key.data(), key.size(), start, stop));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::ZRangeByScore(const std::string& key, double min, double max)
{
  auto min_str = std::to_string(min);
  auto max_str = std::to_string(max);
  auto* reply = static_cast<redisReply*>(
      redisCommand(_ctx, "ZRANGEBYSCORE %b %s %s", key.data(), key.size(), min_str.c_str(), max_str.c_str()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::ZScore(const std::string& key, const std::string& member)
{
  auto* reply = static_cast<redisReply*>(
      redisCommand(_ctx, "ZSCORE %b %b", key.data(), key.size(), member.data(), member.size()));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::ZCard(const std::string& key)
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "ZCARD %b", key.data(), key.size()));
  return RedisReply(reply);
}

PipeLine PooledRedisConnection::NewPipeLine()
{
  return PipeLine{_ctx};
}

RedisReply PooledRedisConnection::Ping()
{
  auto* reply = static_cast<redisReply*>(redisCommand(_ctx, "PING"));
  return RedisReply(reply);
}

RedisReply PooledRedisConnection::Command(const char* format, ...)
{
  va_list app;
  va_start(app, format);
  auto* reply = static_cast<redisReply*>(redisvCommand(_ctx, format, app));
  va_end(app);
  return RedisReply(reply);
}

RedisPool& RedisPool::GetInstance()
{
  static RedisPool instance;
  return instance;
}

void RedisPool::Init(const RedisConfig& config)
{
  _config = config;
  _pool_size = std::min(config.pool_size, MAX_POOL_SIZE);

  std::size_t created = 0;
  try
  {
    for (; created < _pool_size; ++created)
    {
      _slots[created]._ctx = create_connection();
      _slots[created]._in_use.store(false, std::memory_order_relaxed);
    }
  }
  catch (...)
  {
    for (std::size_t i = 0; i < created; ++i)
    {
      redisFree(_slots[i]._ctx);
      _slots[i]._ctx = nullptr;
    }
    _pool_size = 0;
    throw;
  }
  _available.store(_pool_size, std::memory_order_release);

  tools::Logger::getInstance().info("redis pool init successful");
}

PooledRedisConnection RedisPool::GetConnection()
{
  while (true)
  {
    for (std::size_t i = 0; i < _pool_size; ++i)
    {
      bool expected = false;
      if (_slots[i]._in_use.compare_exchange_strong(expected, true, std::memory_order_acquire,
                                                    std::memory_order_relaxed))
      {
        _available.fetch_sub(1, std::memory_order_acquire);

        auto* reply = static_cast<redisReply*>(redisCommand(_slots[i]._ctx, "PING"));
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
        {
          if (reply != nullptr)
          {
            freeReplyObject(reply);
          }

          redisFree(_slots[i]._ctx);
          _slots[i]._ctx = create_connection();
        }
        else
        {
          freeReplyObject(reply);
        }

        return {_slots[i]._ctx, this, i};
      }
    }

    _available.wait(0, std::memory_order_relaxed);
  }
}

void RedisPool::ReleaseConnection(std::size_t slot)
{
  _slots[slot]._in_use.store(false, std::memory_order_release);
  _available.fetch_add(1, std::memory_order_release);
  _available.notify_one();
}

RedisPool::RedisPool() : _config(), _pool_size(0), _available(0)
{
}

RedisPool::~RedisPool()
{
  for (std::size_t i = 0; i < _pool_size; ++i)
  {
    if (_slots[i]._ctx != nullptr)
    {
      redisFree(_slots[i]._ctx);
    }
  }

  tools::Logger::getInstance().info("redis pool closed");
}

redisContext* RedisPool::create_connection() const
{
  struct timeval timeout = {.tv_sec = _config.timeout.count(), .tv_usec = 0};
  redisContext* ctx = redisConnectWithTimeout(_config.host.c_str(), _config.port, timeout);

  if (ctx == nullptr || ctx->err != 0)
  {
    if (ctx != nullptr)
    {
      std::string err_msg = ctx->errstr;
      redisFree(ctx);
      throw std::runtime_error("Failed to connect to Redis: " + err_msg);
    }
    throw std::runtime_error("Failed to allocate Redis context");
  }

  if (!_config.password.empty())
  {
    auto* reply = static_cast<redisReply*>(redisCommand(ctx, "AUTH %s", _config.password.c_str()));
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
      if (reply != nullptr)
      {
        freeReplyObject(reply);
      }
      redisFree(ctx);
      throw std::runtime_error("Failed to authenticate with Redis");
    }
    freeReplyObject(reply);
  }

  if (_config.db_index != 0)
  {
    auto* reply = static_cast<redisReply*>(redisCommand(ctx, "SELECT %lld", static_cast<long long>(_config.db_index)));
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
      if (reply != nullptr)
      {
        freeReplyObject(reply);
      }
      redisFree(ctx);
      throw std::runtime_error("Failed to select Redis database");
    }
    freeReplyObject(reply);
  }

  return ctx;
}

}  // namespace utils