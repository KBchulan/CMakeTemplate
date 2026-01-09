/******************************************************************************
 *
 * @file       rate_limit.hpp
 * @brief      限流中间件
 *
 * @author     KBchulan
 * @date       2025/12/17
 * @history
 ******************************************************************************/

#ifndef RATE_LIMIT_HPP
#define RATE_LIMIT_HPP

#include <chrono>
#include <cstddef>
#include <global/Global.hpp>
#include <random>
#include <string>
#include <tools/Logger.hpp>
#include <utils/UtilsExport.hpp>
#include <utils/pool/redis/redis_pool.hpp>

namespace utils
{

class UTILS_EXPORT RateLimit
{
public:
  static bool Allow(const std::string& cip)
  {
    auto now = static_cast<std::size_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());

    std::string key = global::server::RATE_LIMIT_PREFIX + cip;
    std::string member = std::to_string(now) + "-" + std::to_string(generate_random());

    try
    {
      auto conn = RedisPool::GetInstance().GetConnection();

      // 清理窗口外的过期记录
      conn.Command("ZREMRANGEBYSCORE %s 0 %lld", key.c_str(), now - (global::server::RATE_LIMIT_WINDOW_SIZE * 1000));

      // 先添加当前请求
      conn.ZAdd(key, static_cast<double>(now), member);

      // 设置过期时间
      conn.Expire(key, global::server::RATE_LIMIT_WINDOW_SIZE);

      // 获取当前数量
      auto count = conn.ZCard(key).AsInteger();

      // 超限则回滚
      if (count.has_value() && static_cast<std::size_t>(count.value()) > global::server::RATE_LIMIT_MAX_REQUESTS)
      {
        conn.ZRem(key, member);
        return false;
      }

      return true;
    }
    catch (const std::exception& e)
    {
      tools::Logger::getInstance().error("RateLimit error: {}", e.what());
      return true;
    }
  }

private:
  static std::uint32_t generate_random()
  {
    static thread_local std::mt19937 gen{std::random_device{}()};
    static thread_local std::uniform_int_distribution<std::uint32_t> dist(0, 999999);
    return dist(gen);
  }
};

}  // namespace utils

#endif  // RATE_LIMIT_HPP