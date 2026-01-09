#include "io.hpp"

#include <algorithm>
#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <cstddef>
#include <global/Global.hpp>
#include <iterator>
#include <thread>
#include <tools/Logger.hpp>
#include <vector>

namespace core
{

struct IO::_impl
{
  std::size_t _pool_size;
  std::vector<std::unique_ptr<boost::asio::io_context>> _io_contexts;
  std::vector<std::jthread> _io_threads;
  std::vector<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> _work_guards;
  std::atomic<std::size_t> _next_idx;

  explicit _impl(std::int8_t pool_size) : _pool_size(static_cast<std::size_t>(pool_size)), _next_idx(0)
  {
    _io_contexts.reserve(_pool_size);
    for (std::size_t i = 0; i < _pool_size; ++i)
    {
      _io_contexts.emplace_back(std::make_unique<boost::asio::io_context>());
    }

    _work_guards.reserve(_pool_size);
    std::ranges::transform(_io_contexts, std::back_inserter(_work_guards),
                           [](const auto& ioc) { return boost::asio::make_work_guard(*ioc); });

    _io_threads.reserve(_pool_size);
    std::ranges::transform(_io_contexts, std::back_inserter(_io_threads),
                           [](const auto& ioc) { return std::jthread([&ioc]() { (*ioc).run(); }); });

    tools::Logger::getInstance().info("io pool initialized successful");
  }

  ~_impl()
  {
    // 基于 RAII 回收
    tools::Logger::getInstance().info("io pool closed");
  }
};

IO::IO(std::int8_t pool_size) : _pimpl(std::make_unique<_impl>(pool_size))
{
}

IO::~IO() = default;

IO& IO::GetInstance()
{
  static IO instance(global::server::IO_CONTEXT_POOL_SIZE);
  return instance;
}

boost::asio::io_context& IO::GetIOContext()
{
  // 基于轮询的方式返回 io_context
  auto idx = _pimpl->_next_idx.fetch_add(1, std::memory_order_relaxed) % _pimpl->_pool_size;
  return *_pimpl->_io_contexts[idx];
}

std::size_t IO::GetPoolSize() const
{
  return _pimpl->_pool_size;
}

boost::asio::io_context& IO::GetIOContextAt(std::size_t idx)
{
  return *_pimpl->_io_contexts[idx % _pimpl->_pool_size];
}

}  // namespace core
