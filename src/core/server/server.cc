#include "server.hpp"

#include <sys/socket.h>

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/detail/error_code.hpp>
#include <core/connection/connection.hpp>
#include <core/io/io.hpp>
#include <tools/Logger.hpp>

namespace core
{

// 单个 Acceptor 工作单元，每个 io_context 对应一个
class AcceptorWorker
{
public:
  AcceptorWorker(boost::asio::io_context& ioc, unsigned short port) : _io_context(ioc), _acceptor(ioc), _socket(ioc)
  {
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::any(), port);

    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(boost::asio::socket_base::reuse_address(true));

    // SO_REUSEPORT: 允许多个 socket 绑定同一端口，内核级负载均衡
    int reuse_port = 1;
    setsockopt(_acceptor.native_handle(), SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port));

    _acceptor.bind(endpoint);
    _acceptor.listen(boost::asio::socket_base::max_listen_connections);

    start_accept();
  }

  ~AcceptorWorker()
  {
    boost::system::error_code errc;
    _acceptor.close(errc);
  }

private:
  void start_accept()
  {
    _acceptor.async_accept(_socket, [this](const boost::system::error_code& errc) { on_accept(errc); });
  }

  void on_accept(const boost::system::error_code& errc)
  {
    if (errc == boost::asio::error::operation_aborted)
    {
      return;
    }

    if (!errc)
    {
      std::make_shared<Connection>(std::move(_socket))->Start();
    }
    else
    {
      tools::Logger::getInstance().error("Accept error: {}", errc.message());
    }

    // 准备下一次 accept
    _socket = boost::asio::ip::tcp::socket(_io_context);
    start_accept();
  }

  boost::asio::io_context& _io_context;
  boost::asio::ip::tcp::acceptor _acceptor;
  boost::asio::ip::tcp::socket _socket;
};

struct Server::_impl
{
  unsigned short _port;
  std::vector<std::unique_ptr<AcceptorWorker>> _workers;

  explicit _impl(unsigned short port) : _port(port)
  {
    auto& io_pool = IO::GetInstance();
    auto pool_size = io_pool.GetPoolSize();

    _workers.reserve(pool_size);

    // 每个 io_context 创建一个 acceptor
    for (std::size_t i = 0; i < pool_size; ++i)
    {
      _workers.emplace_back(std::make_unique<AcceptorWorker>(io_pool.GetIOContextAt(i), _port));
    }

    tools::Logger::getInstance().info("HttpServer started on port {} with {} acceptors", _port, pool_size);
  }

  ~_impl()
  {
    tools::Logger::getInstance().info("HttpServer stopped");
  }
};

Server::Server(unsigned short port) : _pimpl(std::make_unique<_impl>(port))
{
}

Server::~Server() = default;

}  // namespace core
