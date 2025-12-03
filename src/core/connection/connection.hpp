/******************************************************************************
 *
 * @file       connection.hpp
 * @brief      单独一个 Http 的连接对象
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <boost/asio/ip/tcp.hpp>
#include <core/CoreExport.hpp>
#include <memory>

namespace core
{

class CORE_EXPORT Connection : public std::enable_shared_from_this<Connection>
{
public:
  explicit Connection(boost::asio::ip::tcp::socket socket);
  ~Connection();

  void Start();

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  Connection(Connection&&) = delete;
  Connection& operator=(Connection&&) = delete;

private:
  struct _impl;
  std::unique_ptr<_impl> _pimpl;
};

}  // namespace core

#endif  // CONNECTION_HPP