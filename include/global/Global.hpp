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

// ============================================================================
// 雪花算法相关常量
// ============================================================================
namespace snowflake
{
// 时间相关
constexpr std::uint64_t EPOCH = 1704067200000ULL;         // 起始时间戳 2024-01-01 00:00:00 UTC
constexpr std::uint64_t MAX_CLOCK_ROLLBACK_MS = 5000ULL;  // 最大允许时钟回拨 5秒
constexpr std::uint64_t MAX_ROLLBACK_WAIT_MS = 100ULL;    // 最大回拨等待时间 100ms

// 位掩码
constexpr std::uint64_t SEQUENCE_MASK = 0xFFF;       // 序列号掩码 12位 (0-4095)
constexpr std::uint64_t WORKER_ID_MASK = 0x3FF;      // WorkerID掩码 10位 (0-1023)
constexpr std::uint64_t TIMESTAMP_MAX_BITS = 41ULL;  // 时间戳最大位数

// 位移量
constexpr std::uint8_t TIMESTAMP_SHIFT = 22;  // 时间戳左移位数
constexpr std::uint8_t WORKER_ID_SHIFT = 12;  // WorkerID左移位数

// 超时设置
constexpr std::chrono::microseconds SLEEP_INTERVAL{100};  // 等待间隔
}  // namespace snowflake

// ============================================================================
// 日志系统相关常量
// ============================================================================
namespace logger
{
constexpr std::size_t MAX_MESSAGE_SIZE = 512;             // 单条日志消息最大长度
constexpr std::size_t QUEUE_CAPACITY = 16384;             // 日志队列容量 2^14
constexpr std::chrono::microseconds FLUSH_INTERVAL{100};  // 刷新等待间隔
}  // namespace logger

}  // namespace global

#endif  // GLOBAL_HPP
