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
#include <memory>
#include <string>
#include <utils/common/type.hpp>

namespace core
{

class CORE_EXPORT Logic
{
public:
  Logic();
  ~Logic();

  static Logic& GetInstance();

  RequestHandleResult HandleGetRequest(const utils::ParsedUrl& url);
  RequestHandleResult HandlePostRequest(const utils::ParsedUrl& url, const std::string& body);
  RequestHandleResult HandlePutRequest(const utils::ParsedUrl& url, const std::string& body);
  RequestHandleResult HandleDeleteRequest(const utils::ParsedUrl& url);

  Logic(const Logic&) = delete;
  Logic& operator=(const Logic&) = delete;
  Logic(Logic&&) = delete;
  Logic& operator=(Logic&&) = delete;

private:
  struct _impl;
  std::unique_ptr<_impl> _pimpl;
};

}  // namespace core

#endif  // LOGIC_HPP