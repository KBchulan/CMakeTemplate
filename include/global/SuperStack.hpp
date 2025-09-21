/******************************************************************************
 *
 * @file       SuperStack.hpp
 * @brief      无锁栈的实现
 *
 * @author     KBchulan
 * @date       2025/09/20
 * @history
 ******************************************************************************/

#ifndef SUPERSTACK_HPP
#define SUPERSTACK_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <random>
#include <thread>
#include <utility>
#include <vector>

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

template <typename T>
struct Node
{
  Node<T>* _next;
  alignas(T) std::array<std::byte, sizeof(T)> _storage;

  T* data() noexcept
  {
    return std::launder(reinterpret_cast<T*>(_storage.data()));
  }
};

template <typename T>
struct TaggedNode
{
  Node<T>* _ptr = nullptr;
  uintptr_t _tag = 0;

  bool operator==(const TaggedNode& other) const
  {
    return this->_ptr == other._ptr && this->_tag == other._tag;
  }
};

template <typename T, size_t Capacity>
class NodePool
{
public:
  NodePool()
  {
    _nodes_storage.reserve(Capacity);
    for (unsigned i = 0; i < Capacity; i++)
    {
      _nodes_storage.emplace_back();
    }
    for (unsigned i = 0; i < Capacity - 1; i++)
    {
      _nodes_storage[i]._next = &_nodes_storage[i + 1];
    }
    _nodes_storage[Capacity - 1]._next = nullptr;
    _head.store({&_nodes_storage[0], 0}, std::memory_order_relaxed);
  }

  Node<T>* acquire()
  {
    BackOff backoff;
    TaggedNode<T> old_head = _head.load(std::memory_order_relaxed);
    while (old_head._ptr)
    {
      TaggedNode<T> new_head;
      new_head._ptr = old_head._ptr->_next;
      new_head._tag = old_head._tag + 1;

      if (_head.compare_exchange_weak(old_head, new_head, std::memory_order_acquire, std::memory_order_relaxed))
      {
        return old_head._ptr;
      }
      backoff.pause();
    }
    return nullptr;
  }

  void release(Node<T>* node)
  {
    BackOff backoff;
    TaggedNode<T> old_head = _head.load(std::memory_order_relaxed);
    TaggedNode<T> new_head;
    do
    {
      node->_next = old_head._ptr;
      new_head._ptr = node;
      new_head._tag = old_head._tag + 1;

      if (_head.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_relaxed))
      {
        break;
      }
      backoff.pause();
    } while (true);
  }

private:
  alignas(CACHE_LINE_SIZE) std::atomic<TaggedNode<T>> _head{};
  std::vector<Node<T>> _nodes_storage;
};

template <typename T, size_t Capacity, size_t EliminationArraySize = 8>
class SuperStack
{
private:
  Node<T>* const POPPER_MARKER = reinterpret_cast<Node<T>*>(this);
  static constexpr int ELIMINATION_ATTEMPTS = 5;

public:
  SuperStack()
  {
    for (auto& slot : _elimination_array)
    {
      slot.store({nullptr, 0}, std::memory_order_relaxed);
    }
  }
  ~SuperStack()
  {
    while (pop().has_value())
    {
    }
  }

  SuperStack(const SuperStack&) = delete;
  SuperStack& operator=(const SuperStack&) = delete;

  template <typename... Args>
  bool emplace(Args&&... args)
  {
    Node<T>* node = _pool.acquire();
    if (!node)
    {
      return false;
    }

    std::construct_at(node->data(), std::forward<Args>(args)...);

    BackOff backoff;
    TaggedNode<T> old_head = _head.load(std::memory_order_relaxed);

    // 1. 直接操作主栈
    for (unsigned i = 0; i < ELIMINATION_ATTEMPTS; ++i)
    {
      TaggedNode<T> new_head;
      node->_next = old_head._ptr;
      new_head._ptr = node;
      new_head._tag = old_head._tag + 1;

      if (_head.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_relaxed))
      {
        return true;
      }
      backoff.pause();
    }

    // 2. 进入退避栈尝试
    if (try_eliminate_push(node))
    {
      return true;
    }

    // 3. 退避栈不行，直接在主栈一直轮询
    backoff.reset();
    old_head = _head.load(std::memory_order_relaxed);
    do
    {
      TaggedNode<T> new_head;
      node->_next = old_head._ptr;
      new_head._ptr = node;
      new_head._tag = old_head._tag + 1;

      if (_head.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_relaxed))
      {
        return true;
      }
      backoff.pause();
    } while (true);
  }

  bool push(const T& value)
  {
    return emplace(value);
  }
  bool push(T&& value)
  {
    return emplace(std::move(value));
  }

  std::optional<T> pop()
  {
    BackOff backoff;
    TaggedNode<T> old_head = _head.load(std::memory_order_relaxed);

    // 1. 主栈pop
    for (unsigned i = 0; i < ELIMINATION_ATTEMPTS; ++i)
    {
      if (!old_head._ptr)
      {
        break;
      }

      TaggedNode<T> new_head;
      new_head._ptr = old_head._ptr->_next;
      new_head._tag = old_head._tag + 1;

      if (_head.compare_exchange_weak(old_head, new_head, std::memory_order_acquire, std::memory_order_relaxed))
      {
        T value = std::move(*old_head._ptr->data());
        std::destroy_at(old_head._ptr->data());
        _pool.release(old_head._ptr);
        return value;
      }
      backoff.pause();
    }

    // 2. 进入退避栈处理
    Node<T>* eliminated_node = try_eliminate_pop();
    if (eliminated_node)
    {
      T value = std::move(*eliminated_node->data());
      std::destroy_at(eliminated_node->data());
      _pool.release(eliminated_node);
      return value;
    }

    // 3. 最后在主栈尝试一下，毕竟有可能是空的
    old_head = _head.load(std::memory_order_relaxed);
    while (old_head._ptr)
    {
      TaggedNode<T> new_head;
      new_head._ptr = old_head._ptr->_next;
      new_head._tag = old_head._tag + 1;

      if (_head.compare_exchange_weak(old_head, new_head, std::memory_order_acquire, std::memory_order_relaxed))
      {
        T value = std::move(*old_head._ptr->data());
        std::destroy_at(old_head._ptr->data());
        _pool.release(old_head._ptr);
        return value;
      }
      backoff.pause();
    }

    return std::nullopt;
  }

private:
  bool try_eliminate_push(Node<T>* node)
  {
    int idx = random_index();
    auto& slot = _elimination_array[idx];

    TaggedNode<T> old_slot = slot.load(std::memory_order_relaxed);
    if (old_slot._ptr == POPPER_MARKER)
    {
      TaggedNode<T> new_slot = {node, old_slot._tag + 1};
      if (slot.compare_exchange_strong(old_slot, new_slot, std::memory_order_acq_rel))
      {
        return true;
      }
    }
    return false;
  }

  Node<T>* try_eliminate_pop()
  {
    int idx = random_index();
    auto& slot = _elimination_array[idx];

    // 如果此时已经有一个 pusher
    TaggedNode<T> old_slot = slot.load(std::memory_order_relaxed);
    if (old_slot._ptr != nullptr && old_slot._ptr != POPPER_MARKER)
    {
      TaggedNode<T> new_slot = {nullptr, old_slot._tag + 1};
      if (slot.compare_exchange_strong(old_slot, new_slot, std::memory_order_acq_rel))
      {
        return old_slot._ptr;
      }
    }

    // 修改标记，并等待一会
    TaggedNode<T> expected_empty = {nullptr, slot.load(std::memory_order_relaxed)._tag};
    TaggedNode<T> new_marker = {POPPER_MARKER, expected_empty._tag + 1};
    if (slot.compare_exchange_strong(expected_empty, new_marker, std::memory_order_acq_rel))
    {
      BackOff backoff;
      for (unsigned i = 0; i < ELIMINATION_ATTEMPTS; ++i)
      {
        TaggedNode<T> current_slot = slot.load(std::memory_order_relaxed);
        if (current_slot._ptr != POPPER_MARKER)
        {
          TaggedNode<T> final_slot = {nullptr, current_slot._tag + 1};
          if (slot.compare_exchange_strong(current_slot, final_slot, std::memory_order_acquire))
          {
            return current_slot._ptr;
          }
        }
        backoff.pause();
      }

      TaggedNode<T> final_empty = {nullptr, slot.load(std::memory_order_relaxed)._tag + 1};
      slot.compare_exchange_strong(new_marker, final_empty, std::memory_order_release);
    }
    return nullptr;
  }

  int random_index()
  {
    thread_local static std::mt19937 generator(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_int_distribution<int> distribution(0, EliminationArraySize - 1);
    return distribution(generator);
  }

  alignas(CACHE_LINE_SIZE) std::atomic<TaggedNode<T>> _head{};
  alignas(CACHE_LINE_SIZE) NodePool<T, Capacity> _pool;
  alignas(CACHE_LINE_SIZE) std::array<std::atomic<TaggedNode<T>>, EliminationArraySize> _elimination_array;
};

}  // namespace global

#endif  // SUPERSTACK_HPP