/******************************************************************************
 *
 * @file       Logger.hpp
 * @brief      高性能异步日志组件
 *
 * @author     KBchulan
 * @date       2025/09/05
 * @history    基于无锁队列的异步日志系统，进一步性能优化
 ******************************************************************************/

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <global/Global.hpp>
#include <global/SuperQueue.hpp>
#include <string>
#include <thread>

namespace tools
{

enum class LogLevel : std::uint8_t
{
  TRACE,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL
};

// 日志消息结构，用于预分配内存
struct LogMessage
{
  LogMessage() : _level(LogLevel::INFO)
  {
    _formatted_message.fill('\0');
  }

  template <typename... Args>
  LogMessage(LogLevel lev, const fmt::text_style& sty, const std::string& format, Args&&... args)
      : _level(lev), _style(sty), _timestamp(std::chrono::system_clock::now())
  {
    auto result = fmt::vformat_to_n(_formatted_message.data(), global::logger::MAX_MESSAGE_SIZE - 1, format,
                                    fmt::make_format_args(args...));
    _message_length = std::min(result.size, global::logger::MAX_MESSAGE_SIZE - 1);
    _formatted_message[_message_length] = '\0';
  }

  LogLevel _level;
  fmt::text_style _style;
  std::chrono::system_clock::time_point _timestamp;

  size_t _message_length{0};
  std::array<char, global::logger::MAX_MESSAGE_SIZE> _formatted_message;
};

class Logger
{
public:
  static Logger& getInstance()
  {
    static Logger instance;
    return instance;
  }

  ~Logger()
  {
    _should_stop.store(true, std::memory_order_relaxed);

    _pending_count.fetch_add(1, std::memory_order_release);
    _pending_count.notify_one();

    if (_worker_thread.joinable())
    {
      _worker_thread.join();
    }
  }

  // 普通打印
  template <typename... Args>
  [[maybe_unused]] void print(const std::string& format, Args&&... args) const
  {
    if (_log_queue.emplace(LogLevel::INFO, fmt::text_style{}, format, std::forward<Args>(args)...))
    {
      _pending_count.fetch_add(1, std::memory_order_release);
      _pending_count.notify_one();
    }
  }

  // 指定样式进行打印
  template <typename... Args>
  [[maybe_unused]] void print(const fmt::text_style& style, const std::string& format, Args&&... args) const
  {
    if (_log_queue.emplace(LogLevel::INFO, style, format, std::forward<Args>(args)...))
    {
      _pending_count.fetch_add(1, std::memory_order_release);
      _pending_count.notify_one();
    }
  }

  // 各个类型的日志
  template <typename... Args>
  [[maybe_unused]] void info(const std::string& format, Args&&... args) const
  {
    if (_log_queue.emplace(LogLevel::INFO, fmt::fg(fmt::color::green), format, std::forward<Args>(args)...))
    {
      _pending_count.fetch_add(1, std::memory_order_release);
      _pending_count.notify_one();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void warning(const std::string& format, Args&&... args) const
  {
    if (_log_queue.emplace(LogLevel::WARNING, fmt::fg(fmt::color::yellow), format, std::forward<Args>(args)...))
    {
      _pending_count.fetch_add(1, std::memory_order_release);
      _pending_count.notify_one();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void error(const std::string& format, Args&&... args) const
  {
    if (_log_queue.emplace(LogLevel::ERROR, fmt::fg(fmt::color::red), format, std::forward<Args>(args)...))
    {
      _pending_count.fetch_add(1, std::memory_order_release);
      _pending_count.notify_one();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void trace(const std::string& format, Args&&... args) const
  {
    if (_log_queue.emplace(LogLevel::TRACE, fmt::fg(fmt::color::gray), format, std::forward<Args>(args)...))
    {
      _pending_count.fetch_add(1, std::memory_order_release);
      _pending_count.notify_one();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void debug(const std::string& format, Args&&... args) const
  {
    if (_log_queue.emplace(LogLevel::DEBUG, fmt::fg(fmt::color::blue), format, std::forward<Args>(args)...))
    {
      _pending_count.fetch_add(1, std::memory_order_release);
      _pending_count.notify_one();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void fatal(const std::string& format, Args&&... args) const
  {
    if (_log_queue.emplace(LogLevel::FATAL, fmt::fg(fmt::color::red), format, std::forward<Args>(args)...))
    {
      _pending_count.fetch_add(1, std::memory_order_release);
      _pending_count.notify_one();
    }
  }

  // 获取队列长度
  [[nodiscard]] size_t queueSize() const
  {
    return _pending_count.load(std::memory_order_acquire);
  }

  // 强制刷新，等待队列清空
  [[maybe_unused]] void flush() const
  {
    while (queueSize() > 0)
    {
      std::this_thread::sleep_for(global::logger::FLUSH_INTERVAL);
    }
  }

private:
  using LogQueue = global::SuperQueue<LogMessage, global::logger::QUEUE_CAPACITY>;

  mutable LogQueue _log_queue;
  std::jthread _worker_thread;
  std::atomic<bool> _should_stop{false};
  mutable std::atomic<size_t> _pending_count{0};

  Logger()
  {
    _worker_thread = std::jthread([this] -> void { _logWorker(); });
    fmt::print(fmt::fg(fmt::color::green), "[Init] Async Logger initialized successfully...\n");
  }

  void _logWorker() noexcept
  {
    LogMessage msg;

    while (true)
    {
      size_t expected = 0;
      _pending_count.wait(expected, std::memory_order_acquire);

      if (_should_stop.load(std::memory_order_relaxed))
      {
        break;
      }

      // 批量处理消息
      while (_log_queue.pop(msg))
      {
        fmt::print(msg._style, "[{}] [{}] {}\n", _getLevelString(msg._level), _formatTimestamp(msg._timestamp),
                   msg._formatted_message.data());

        _pending_count.fetch_sub(1, std::memory_order_acq_rel);
      }
    }

    // 清空剩余日志
    while (_log_queue.pop(msg))
    {
      fmt::print(msg._style, "[{}] [{}] {}\n", _getLevelString(msg._level), _formatTimestamp(msg._timestamp),
                 msg._formatted_message.data());
    }
  }

  static constexpr std::string_view _getLevelString(LogLevel level) noexcept
  {
    switch (level)
    {
      case LogLevel::TRACE:
        return "TRACE";
      case LogLevel::DEBUG:
        return "DEBUG";
      case LogLevel::INFO:
        return "INFO";
      case LogLevel::WARNING:
        return "WARNING";
      case LogLevel::ERROR:
        return "ERROR";
      case LogLevel::FATAL:
        return "FATAL";
      default:
        return "UNKNOWN";
    }
  }

  static std::string _formatTimestamp(const std::chrono::system_clock::time_point& timestamp) noexcept
  {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm time_info{};

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&time_info, &time_t);
#else
    localtime_r(&time_t, &time_info);
#endif

    return fmt::format("{:04d}-{:02d}-{:02d}-{:02d}:{:02d}", time_info.tm_year + 1900, time_info.tm_mon + 1,
                       time_info.tm_mday, time_info.tm_hour, time_info.tm_min);
  }
};

}  // namespace tools

#endif  // LOGGER_HPP