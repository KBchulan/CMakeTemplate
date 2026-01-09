#include "business.hpp"

#include <global/Global.hpp>
#include <tools/Logger.hpp>

namespace core
{

struct Business::_impl
{
  boost::asio::thread_pool _pool;

  _impl() : _pool(global::server::BUSINESS_POOL_SIZE)
  {
    tools::Logger::getInstance().info("business pool init successful");
  }

  ~_impl()
  {
    _pool.join();
    tools::Logger::getInstance().info("business pool closed");
  }
};

Business::Business() : _pimpl(std::make_unique<_impl>())
{
}

Business::~Business() = default;

Business& Business::GetInstance()
{
  static Business instance;
  return instance;
}

boost::asio::thread_pool& Business::GetBusinessPool()
{
  return _pimpl->_pool;
}

}  // namespace core