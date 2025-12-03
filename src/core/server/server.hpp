/******************************************************************************
 *
 * @file       server.hpp
 * @brief      核心入口，用于监听所有的连接信息
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio/io_context.hpp>
#include <core/CoreExport.hpp>
#include <memory>

namespace core
{

class CORE_EXPORT Server
{
public:
  Server(boost::asio::io_context& ioc, unsigned short port);
  ~Server();

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;
  Server(Server&&) = delete;
  Server& operator=(Server&&) = delete;

private:
  struct _impl;
  std::unique_ptr<_impl> _pimpl;
};

}  // namespace core

#endif  // SERVER_HPP