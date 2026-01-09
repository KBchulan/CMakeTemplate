/******************************************************************************
 *
 * @file       business.hpp
 * @brief      业务线程池，用于投递业务，与网络层解耦和
 *
 * @author     KBchulan
 * @date       2025/12/07
 * @history
 ******************************************************************************/

#ifndef BUSINESS_HPP
#define BUSINESS_HPP

#include <boost/asio/thread_pool.hpp>
#include <core/CoreExport.hpp>
#include <memory>

namespace core
{

class CORE_EXPORT Business
{
public:
  static Business& GetInstance();

  boost::asio::thread_pool& GetBusinessPool();

  Business(const Business&) = delete;
  Business& operator=(const Business&) = delete;
  Business(Business&&) = delete;
  Business& operator=(Business&&) = delete;

private:
  Business();
  ~Business();

  struct _impl;
  std::unique_ptr<_impl> _pimpl;
};

}  // namespace core

#endif  // BUSINESS_HPP