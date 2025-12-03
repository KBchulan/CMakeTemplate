#include "server.hpp"

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>
#include <core/connection/connection.hpp>
#include <functional>
#include <memory>
#include <tools/Defer.hpp>
#include <tools/Logger.hpp>

namespace core
{

struct Server::_impl
{
  boost::asio::io_context& _io_context;
  unsigned short _port;
  boost::asio::ip::tcp::acceptor _acceptor;
  boost::asio::ip::tcp::socket _socket;
  std::function<void(const boost::system::error_code&)> _accept_handler;

  void start_accept()
  {
    _acceptor.async_accept(_socket, _accept_handler);
  }

  _impl(boost::asio::io_context& ioc, unsigned short port)
      : _io_context(ioc),
        _port(port),
        _acceptor(_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::any(), _port)),
        _socket(_io_context)
  {
    tools::Logger::getInstance().info("Http server started on port {}", _port);

    _accept_handler = [this](const boost::system::error_code& errc) -> void
    {
      defer(start_accept());
      defer(_socket = boost::asio::ip::tcp::socket(_io_context));

      if (errc)
      {
        // 出错就放弃当前连接，并监听下一个连接
        tools::Logger::getInstance().error("Accept connection error, error msg: {}", errc.message());
        return;
      }

      // 创建一个连接对象来处理这个连接，并继续监听其他连接
      std::make_shared<Connection>(std::move(_socket))->Start();
    };

    // 首次开始监听连接
    start_accept();
  }
};

Server::Server(boost::asio::io_context& ioc, unsigned short port) : _pimpl(std::make_unique<_impl>(ioc, port))
{
}

Server::~Server() = default;

}  // namespace core