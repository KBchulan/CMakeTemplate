/******************************************************************************
 *
 * @file       logic.hpp
 * @brief      逻辑系统的单例实现
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef LOGIC_HPP
#define LOGIC_HPP

#include <core/CoreExport.hpp>
#include <utils/common/type.hpp>
#include <utils/context/context.hpp>

namespace core
{

class CORE_EXPORT Logic
{
public:
  static Logic& GetInstance();

  RequestHandleResult HandleGetRequest(const utils::Context& ctx);
  RequestHandleResult HandlePostRequest(const utils::Context& ctx);
  RequestHandleResult HandlePutRequest(const utils::Context& ctx);
  RequestHandleResult HandleDeleteRequest(const utils::Context& ctx);

  Logic(const Logic&) = delete;
  Logic& operator=(const Logic&) = delete;
  Logic(Logic&&) = delete;
  Logic& operator=(Logic&&) = delete;

private:
  Logic();
  ~Logic();

  struct _impl;
  std::unique_ptr<_impl> _pimpl;
};

}  // namespace core

#endif  // LOGIC_HPP