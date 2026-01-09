/******************************************************************************
 *
 * @file       io.hpp
 * @brief      封装 io-context 的连接池子
 *
 * @author     KBchulan
 * @date       2025/12/07
 * @history
 ******************************************************************************/

#ifndef IO_HPP
#define IO_HPP

#include <boost/asio/io_context.hpp>
#include <core/CoreExport.hpp>
#include <cstdint>
#include <memory>

namespace core
{

class CORE_EXPORT IO
{
public:
  static IO& GetInstance();

  // 获取轮询的 io_context
  [[nodiscard]] boost::asio::io_context& GetIOContext();

  // 获取池大小
  [[nodiscard]] std::size_t GetPoolSize() const;

  // 根据索引获取 io_context
  [[nodiscard]] boost::asio::io_context& GetIOContextAt(std::size_t idx);

  IO(const IO&) = delete;
  IO& operator=(const IO&) = delete;
  IO(IO&&) = delete;
  IO& operator=(IO&&) = delete;

private:
  explicit IO(std::int8_t pool_size);
  ~IO();

  struct _impl;
  std::unique_ptr<_impl> _pimpl;
};

}  // namespace core

#endif  // IO_HPP