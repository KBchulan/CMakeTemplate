/******************************************************************************
 *
 * @file       SuperQueue.hpp
 * @brief      高性能无锁多生产者多消费者队列
 *
 * @author     KBchulan
 * @date       2025/09/05
 * @history
 ******************************************************************************/

#ifndef SUPER_QUEUE_HPP
#define SUPER_QUEUE_HPP

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace global
{

#if defined(__cpp_lib_hardware_interference_size)
constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
#else
constexpr size_t CACHE_LINE_SIZE = 64;
#endif

class BackOff
{
public:
  void pause()
  {
    for (int i = 0; i < _count; i++)
    {
      cpu_pause();
    }
    if (_count < 1024)
    {
      _count *= 2;
    }
  }

  [[maybe_unused]] void reset()
  {
    _count = 1;
  }

private:
  static void cpu_pause()
  {
// x86 架构
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
// ARM 架构
#elif defined(__arm__) || defined(__aarch64__)
    __asm__ __volatile__("yield");
// 其他架构
#else
    std::this_thread::yield();
#endif
  }

  int _count = 1;
};

template <typename T, size_t Capacity>
class SuperQueue
{
private:
  // 确保容量是2的幂，便于位运算优化
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
  static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>,
                "T must can be move or copy construction");
  static_assert(std::is_nothrow_move_constructible_v<T> || std::is_nothrow_copy_constructible_v<T>,
                "the copy or move shouldn't throw error");

  static constexpr size_t CACHE_LINE_SIZE = 64;

  struct alignas(CACHE_LINE_SIZE) Slot
  {
    std::atomic<size_t> _sequence{0};
    alignas(T) std::array<std::byte, sizeof(T)> _storage;

    T* data() noexcept
    {
      return std::launder(reinterpret_cast<T*>(_storage.data()));
    }
  };

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> _enqueue_pos{0};
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> _dequeue_pos{0};

  std::array<Slot, Capacity> _buffer;

public:
  SuperQueue()
  {
    // 初始化每个slot的序列号
    for (size_t i = 0; i < Capacity; ++i)
    {
      _buffer[i]._sequence.store(i, std::memory_order_relaxed);
    }
  }

  ~SuperQueue()
  {
    // 析构剩余的元素
    size_t front = _dequeue_pos.load(std::memory_order_relaxed);
    size_t back = _enqueue_pos.load(std::memory_order_relaxed);

    while (front != back)
    {
      size_t pos = front & (Capacity - 1);  // 位运算取模
      if (_buffer[pos]._sequence.load(std::memory_order_acquire) == front + 1)
      {
        std::destroy_at(_buffer[pos].data());
      }
      ++front;
    }
  }

  template <typename... Args>
  bool emplace(Args&&... args)
  {
    Slot* slot;
    size_t pos = _enqueue_pos.load(std::memory_order_relaxed);
    BackOff backoff;

    while (true)
    {
      slot = &_buffer[pos & (Capacity - 1)];
      size_t seq = slot->_sequence.load(std::memory_order_acquire);
      intptr_t diff = (intptr_t)seq - (intptr_t)pos;

      if (diff == 0)
      {
        // 该位置可以插入，尝试占据这个位置
        if (_enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        {
          break;
        }
        backoff.pause();
      }
      else if (diff < 0)
      {
        // 队列满
        return false;
      }
      else
      {
        // 其他生产者已经占用了这个位置
        pos = _enqueue_pos.load(std::memory_order_relaxed);
        backoff.pause();
      }
    }

    // 在占据的槽中构造元素
    std::construct_at(slot->data(), std::forward<Args>(args)...);

    // 更新序列号，使数据对消费者可见
    slot->_sequence.store(pos + 1, std::memory_order_release);

    return true;
  }

  bool pop(T& result)
  {
    Slot* slot;
    size_t pos = _dequeue_pos.load(std::memory_order_relaxed);
    BackOff backoff;

    while (true)
    {
      slot = &_buffer[pos & (Capacity - 1)];
      size_t seq = slot->_sequence.load(std::memory_order_acquire);
      intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

      if (diff == 0)
      {
        // 尝试更新出队位置
        if (_dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
        {
          break;
        }
        backoff.pause();
      }
      else if (diff < 0)
      {
        // 队列空
        return false;
      }
      else
      {
        // 其他消费者已经占用了这个位置
        pos = _dequeue_pos.load(std::memory_order_relaxed);
        backoff.pause();
      }
    }

    // 读取数据
    result = std::move(*slot->data());
    std::destroy_at(slot->data());

    // 更新序列号，使位置对生产者可用
    slot->_sequence.store(pos + Capacity, std::memory_order_release);

    return true;
  }

  // 获取当前队列大小
  [[nodiscard]] size_t size() const noexcept
  {
    size_t head = _enqueue_pos.load(std::memory_order_relaxed);
    size_t tail = _dequeue_pos.load(std::memory_order_relaxed);
    return head - tail;
  }

  // 检查是否为空
  [[nodiscard]] bool empty() const noexcept
  {
    return size() == 0;
  }
};

}  // namespace global

#endif  // SUPER_QUEUE_HPP