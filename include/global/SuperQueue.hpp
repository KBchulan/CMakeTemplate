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
#include <memory>
#include <utility>

namespace global
{

template <typename T, size_t Capacity>
class SuperQueue
{
private:
  // 确保容量是2的幂，便于位运算优化
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

  static constexpr size_t _cache_line_size = 64;

  struct alignas(_cache_line_size) Slot
  {
    std::atomic<size_t> _sequence{0};
    T _data;
  };

  alignas(_cache_line_size) std::atomic<size_t> _enqueue_pos{0};
  alignas(_cache_line_size) std::atomic<size_t> _dequeue_pos{0};

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
      if (_buffer[pos]._sequence.load(std::memory_order_relaxed) == front + 1)
      {
        std::destroy_at(&_buffer[pos]._data);
      }
      ++front;
    }
  }

  template <typename... Args>
  bool emplace(Args &&...args)
  {
    Slot *slot;
    size_t pos = _enqueue_pos.load(std::memory_order_relaxed);

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
      }
    }

    // 在占据的槽中构造元素
    std::construct_at(&slot->_data, std::forward<Args>(args)...);

    // 更新序列号，使数据对消费者可见
    slot->_sequence.store(pos + 1, std::memory_order_release);

    return true;
  }

  bool pop(T &result)
  {
    Slot *slot;
    size_t pos = _dequeue_pos.load(std::memory_order_relaxed);

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
      }
    }

    // 读取数据
    result = std::move(slot->_data);
    std::destroy_at(&slot->_data);

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