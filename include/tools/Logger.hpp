/******************************************************************************
 *
 * @file       Logger.hpp
 * @brief      高性能异步日志组件
 *
 * @author     KBchulan
 * @date       2025/09/05
 * @history    基于无锁队列的异步日志系统
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
#include <config/Config.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
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
  FATAL,
  COUNT
};

constexpr std::size_t LOG_LEVEL_COUNT = static_cast<std::size_t>(LogLevel::COUNT);

// 日志消息结构
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

// ========================
// 每级别独立通道
// ========================
template <std::size_t Capacity>
struct LevelChannel
{
  using Queue = global::SuperQueue<LogMessage, Capacity>;

  Queue _queue;
  mutable std::atomic<std::size_t> _pending{0};
  std::atomic<bool> _should_stop{false};
  std::jthread _worker;

  std::ofstream _file;
  std::size_t _batch_threshold{1};
  std::chrono::milliseconds _flush_interval{0};

  // 通知有新消息
  void notify() const
  {
    const auto prev_pending = _pending.fetch_add(1, std::memory_order_release);
    if (prev_pending == 0)
    {
      _pending.notify_one();
    }
  }
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
    _stopAll();
  }

  template <typename... Args>
  [[maybe_unused]] void print(const std::string& format, Args&&... args) const
  {
    if (_ch_info._queue.emplace(LogLevel::INFO, fmt::text_style{}, format, std::forward<Args>(args)...))
    {
      _ch_info.notify();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void print(const fmt::text_style& style, const std::string& format, Args&&... args) const
  {
    if (_ch_info._queue.emplace(LogLevel::INFO, style, format, std::forward<Args>(args)...))
    {
      _ch_info.notify();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void trace(const std::string& format, Args&&... args) const
  {
    if (_ch_trace._queue.emplace(LogLevel::TRACE, fmt::fg(fmt::color::gray), format, std::forward<Args>(args)...))
    {
      _ch_trace.notify();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void debug(const std::string& format, Args&&... args) const
  {
    if (_ch_debug._queue.emplace(LogLevel::DEBUG, fmt::fg(fmt::color::blue), format, std::forward<Args>(args)...))
    {
      _ch_debug.notify();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void info(const std::string& format, Args&&... args) const
  {
    if (_ch_info._queue.emplace(LogLevel::INFO, fmt::fg(fmt::color::green), format, std::forward<Args>(args)...))
    {
      _ch_info.notify();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void warning(const std::string& format, Args&&... args) const
  {
    if (_ch_warn._queue.emplace(LogLevel::WARNING, fmt::fg(fmt::color::yellow), format, std::forward<Args>(args)...))
    {
      _ch_warn.notify();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void error(const std::string& format, Args&&... args) const
  {
    if (_ch_error._queue.emplace(LogLevel::ERROR, fmt::fg(fmt::color::red), format, std::forward<Args>(args)...))
    {
      _ch_error.notify();
    }
  }

  template <typename... Args>
  [[maybe_unused]] void fatal(const std::string& format, Args&&... args) const
  {
    if (_ch_fatal._queue.emplace(LogLevel::FATAL, fmt::fg(fmt::color::red), format, std::forward<Args>(args)...))
    {
      _ch_fatal.notify();
    }
  }

  // 所有通道 pending 之和
  [[nodiscard]] size_t queueSize() const
  {
    return _ch_trace._pending.load(std::memory_order_acquire) + _ch_debug._pending.load(std::memory_order_acquire) +
           _ch_info._pending.load(std::memory_order_acquire) + _ch_warn._pending.load(std::memory_order_acquire) +
           _ch_error._pending.load(std::memory_order_acquire) + _ch_fatal._pending.load(std::memory_order_acquire);
  }

  // 等待全部通道清空
  [[maybe_unused]] void flush() const
  {
    while (queueSize() > 0)
    {
      std::this_thread::sleep_for(global::logger::FLUSH_POLL_INTERVAL);
    }
  }

private:
  // 6 个独立通道，容量各不相同
  mutable LevelChannel<global::logger::QUEUE_CAPACITY_TRACE> _ch_trace;
  mutable LevelChannel<global::logger::QUEUE_CAPACITY_DEBUG> _ch_debug;
  mutable LevelChannel<global::logger::QUEUE_CAPACITY_INFO> _ch_info;
  mutable LevelChannel<global::logger::QUEUE_CAPACITY_WARN> _ch_warn;
  mutable LevelChannel<global::logger::QUEUE_CAPACITY_ERROR> _ch_error;
  mutable LevelChannel<global::logger::QUEUE_CAPACITY_FATAL> _ch_fatal;

  Logger()
  {
    _initChannelConfig();
    _initLogFiles();
    _startWorkers();
  }

  // ---- 配置 ----

  void _initChannelConfig()
  {
    _ch_trace._batch_threshold = global::logger::BATCH_THRESHOLD_TRACE;
    _ch_trace._flush_interval = global::logger::FLUSH_INTERVAL_TRACE;

    _ch_debug._batch_threshold = global::logger::BATCH_THRESHOLD_DEBUG;
    _ch_debug._flush_interval = global::logger::FLUSH_INTERVAL_DEBUG;

    _ch_info._batch_threshold = global::logger::BATCH_THRESHOLD_INFO;
    _ch_info._flush_interval = global::logger::FLUSH_INTERVAL_INFO;

    _ch_warn._batch_threshold = global::logger::BATCH_THRESHOLD_WARN;
    _ch_warn._flush_interval = global::logger::FLUSH_INTERVAL_WARN;

    _ch_error._batch_threshold = global::logger::BATCH_THRESHOLD_ERROR;
    _ch_error._flush_interval = global::logger::FLUSH_INTERVAL_ERROR;

    _ch_fatal._batch_threshold = global::logger::BATCH_THRESHOLD_FATAL;
    _ch_fatal._flush_interval = global::logger::FLUSH_INTERVAL_FATAL;
  }

  void _initLogFiles()
  {
    if constexpr (!global::logger::ENABLE_FILE_LOG)
    {
      return;
    }

    std::filesystem::path log_dir(LOGS_DIR);
    if (!std::filesystem::exists(log_dir))
    {
      std::filesystem::create_directories(log_dir);
    }

    auto openFile = [&](auto& channel, LogLevel level)
    {
      auto file_path = log_dir / fmt::format("{}.log", _getLevelString(level));
      channel._file.open(file_path, std::ios::app);
    };

    openFile(_ch_trace, LogLevel::TRACE);
    openFile(_ch_debug, LogLevel::DEBUG);
    openFile(_ch_info, LogLevel::INFO);
    openFile(_ch_warn, LogLevel::WARNING);
    openFile(_ch_error, LogLevel::ERROR);
    openFile(_ch_fatal, LogLevel::FATAL);
  }

  // ---- 工作线程 ----

  void _startWorkers()
  {
    auto launchWorker = [this](auto& channel)
    { channel._worker = std::jthread([this, &channel] { _workerLoop(channel); }); };

    launchWorker(_ch_trace);
    launchWorker(_ch_debug);
    launchWorker(_ch_info);
    launchWorker(_ch_warn);
    launchWorker(_ch_error);
    launchWorker(_ch_fatal);
  }

  template <std::size_t Cap>
  void _workerLoop(LevelChannel<Cap>& channel) noexcept
  {
    LogMessage msg;
    std::size_t since_last_flush = 0;
    auto last_flush_time = std::chrono::steady_clock::now();

    while (true)
    {
      // 等待通知或超时
      std::size_t expected = 0;
      channel._pending.wait(expected, std::memory_order_acquire);

      if (channel._should_stop.load(std::memory_order_relaxed))
      {
        break;
      }

      // 批量消费
      while (channel._queue.pop(msg))
      {
        _processMessage(msg, channel._file);
        channel._pending.fetch_sub(1, std::memory_order_acq_rel);
        ++since_last_flush;

        // 达到批量阈值 → 刷盘
        if (since_last_flush >= channel._batch_threshold)
        {
          _flushFile(channel._file);
          since_last_flush = 0;
          last_flush_time = std::chrono::steady_clock::now();
        }
      }

      // 定时刷盘：即使没达到阈值，超过间隔也刷
      auto now = std::chrono::steady_clock::now();
      if (since_last_flush > 0 &&
          (channel._flush_interval.count() == 0 || now - last_flush_time >= channel._flush_interval))
      {
        _flushFile(channel._file);
        since_last_flush = 0;
        last_flush_time = now;
      }
    }

    // 停止前清空残余
    while (channel._queue.pop(msg))
    {
      _processMessage(msg, channel._file);
    }
    _flushFile(channel._file);
  }

  static void _processMessage(const LogMessage& msg, std::ofstream& file)
  {
    // 统一格式化一次，控制台与文件复用同一份文本
    fmt::memory_buffer line_buffer;
    fmt::format_to(std::back_inserter(line_buffer), "[{}] [{}] {}\n", _getLevelString(msg._level),
                   _formatTimestamp(msg._timestamp), msg._formatted_message.data());
    const auto line_view = std::string_view(line_buffer.data(), line_buffer.size());

    // 控制台输出
    if constexpr (global::logger::ENABLE_CONSOLE_LOG)
    {
      fmt::print(msg._style, "{}", line_view);
    }

    // 文件输出
    if constexpr (global::logger::ENABLE_FILE_LOG)
    {
      if (file.is_open())
      {
        file.write(line_view.data(), static_cast<std::streamsize>(line_view.size()));
      }
    }
  }

  static void _flushFile(std::ofstream& file)
  {
    if constexpr (global::logger::ENABLE_FILE_LOG)
    {
      if (file.is_open())
      {
        file.flush();
      }
    }
  }

  void _stopAll()
  {
    auto stopOne = [](auto& channel)
    {
      channel._should_stop.store(true, std::memory_order_relaxed);

      channel._pending.fetch_add(1, std::memory_order_release);
      channel._pending.notify_one();

      if (channel._worker.joinable())
      {
        channel._worker.join();
      }
    };

    stopOne(_ch_trace);
    stopOne(_ch_debug);
    stopOne(_ch_info);
    stopOne(_ch_warn);
    stopOne(_ch_error);
    stopOne(_ch_fatal);
  }

  // ---- 工具函数 ----

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

    return fmt::format("{:04d}-{:02d}-{:02d}-{:02d}:{:02d}:{:02d}", time_info.tm_year + 1900, time_info.tm_mon + 1,
                       time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
  }
};

}  // namespace tools

#endif  // LOGGER_HPP
