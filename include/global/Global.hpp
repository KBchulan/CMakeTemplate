/******************************************************************************
 *
 * @file       Global.hpp
 * @brief      这里写全局的声明配置，宏定义集中处，以及各个文件的魔法数字
 *
 * @author     KBchulan
 * @date       2025/04/03
 * @history
 ******************************************************************************/

#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace global
{

// ================
// 雪花算法相关常量
// ================
namespace snowflake
{
constexpr std::uint64_t EPOCH = 1704067200000ULL;         // 起始时间戳 2024-01-01 00:00:00 UTC
constexpr std::uint64_t MAX_CLOCK_ROLLBACK_MS = 5000ULL;  // 最大允许时钟回拨 5秒
constexpr std::uint64_t MAX_ROLLBACK_WAIT_MS = 100ULL;    // 最大回拨等待时间 100ms
constexpr std::chrono::microseconds SLEEP_INTERVAL{100};  // 等待间隔
}  // namespace snowflake

// ================
// 日志系统相关常量
// ================
namespace logger
{
constexpr std::size_t MAX_MESSAGE_SIZE = 512;  // 单条日志消息最大长度
constexpr bool ENABLE_FILE_LOG = true;         // 是否启用文件日志
constexpr bool ENABLE_CONSOLE_LOG = true;      // 是否启用控制台日志

// 每级别队列容量（必须是 2 的幂）
constexpr std::size_t QUEUE_CAPACITY_TRACE = 8192;  // TRACE  吞吐型
constexpr std::size_t QUEUE_CAPACITY_DEBUG = 8192;  // DEBUG  吞吐型
constexpr std::size_t QUEUE_CAPACITY_INFO = 16384;  // INFO   最大
constexpr std::size_t QUEUE_CAPACITY_WARN = 4096;   // WARN   中等
constexpr std::size_t QUEUE_CAPACITY_ERROR = 2048;  // ERROR  实时型
constexpr std::size_t QUEUE_CAPACITY_FATAL = 1024;  // FATAL  实时型

// 批量刷盘阈值，累积 x 条后触发一次 flush
constexpr std::size_t BATCH_THRESHOLD_TRACE = 64;
constexpr std::size_t BATCH_THRESHOLD_DEBUG = 64;
constexpr std::size_t BATCH_THRESHOLD_INFO = 32;
constexpr std::size_t BATCH_THRESHOLD_WARN = 8;
constexpr std::size_t BATCH_THRESHOLD_ERROR = 4;
constexpr std::size_t BATCH_THRESHOLD_FATAL = 4;

// 各级别定时刷盘间隔
using namespace std::chrono_literals;
constexpr std::chrono::milliseconds FLUSH_INTERVAL_TRACE{500};
constexpr std::chrono::milliseconds FLUSH_INTERVAL_DEBUG{500};
constexpr std::chrono::milliseconds FLUSH_INTERVAL_INFO{200};
constexpr std::chrono::milliseconds FLUSH_INTERVAL_WARN{50};
constexpr std::chrono::milliseconds FLUSH_INTERVAL_ERROR{2};
constexpr std::chrono::milliseconds FLUSH_INTERVAL_FATAL{2};

// flush() 轮询等待间隔
constexpr std::chrono::microseconds FLUSH_POLL_INTERVAL{100};
}  // namespace logger

// ================
// 服务器相关常量
// ================
namespace server
{

constexpr unsigned short DEFAULT_SERVER_PORT = 10001;  // 默认服务器端口
constexpr std::int8_t IO_CONTEXT_POOL_SIZE = 8;        // io_context 池子大小
constexpr std::int8_t BUSINESS_POOL_SIZE = 8;          // 业务池子大小
constexpr std::uint16_t MAX_FLATBUFFER_SIZE = 8192;    // 最大扁平化缓冲区大小 8KB

constexpr const char* DB_HOST = "127.0.0.1";  // 数据库主机地址
constexpr std::uint16_t DB_PORT = 3306;       // 数据库端口
constexpr const char* DB_USER = "root";       // 数据库用户名
constexpr const char* DB_PASSWORD = "whx";    // 数据库密码
constexpr const char* DB_NAME = "chatroom";   // 数据库名称
constexpr std::size_t DB_MAX_POOL_SIZE = 16;  // 数据库最大连接池大小

constexpr const char* REDIS_HOST = "127.0.0.1";  // Redis 主机地址
constexpr std::uint16_t REDIS_PORT = 6379;       // Redis 端口
constexpr const char* REDIS_PASSWORD = "whx";    // Redis 密码
constexpr std::size_t REDIS_DB_INDEX = 0;        // Redis 数据库索引
constexpr std::size_t REDIS_MAX_POOL_SIZE = 16;  // Redis 最大连接池大小
constexpr std::size_t REDIS_TIMEOUT = 3;         // Redis 连接超时时间

constexpr auto JWT_DEFAULT_SECRET = "CMakeTemplate-Secret-Key-2025";
constexpr auto JWT_ISSUER = "CMakeTemplate-GateWay";
constexpr auto JWT_EXPIRATION_TIME = 7 * 24;

constexpr const char* RATE_LIMIT_PREFIX = "rate_limit:";  // 限流前缀
constexpr std::size_t RATE_LIMIT_WINDOW_SIZE = 60;        // 窗口大小 60秒
constexpr std::size_t RATE_LIMIT_MAX_REQUESTS = 100;      // 每分钟最大请求数

}  // namespace server

}  // namespace global

#endif  // GLOBAL_HPP
