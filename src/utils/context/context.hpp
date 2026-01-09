/******************************************************************************
 *
 * @file       context.hpp
 * @brief      自定义网络上下文
 *
 * @author     KBchulan
 * @date       2025/12/05
 * @history
 ******************************************************************************/

#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <any>
#include <memory>
#include <utils/UtilsExport.hpp>
#include <utils/url/url.hpp>

namespace utils
{

class UTILS_EXPORT Context
{
public:
  Context(ParsedUrl& url, std::string& body);
  ~Context();

  void Set(const std::string& key, std::any value);
  [[nodiscard]] std::any Get(const std::string& key) const;

  [[nodiscard]] const ParsedUrl& GetUrl() const;
  [[nodiscard]] const std::string& GetBody() const;

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  Context(Context&&) noexcept;
  Context& operator=(Context&&) noexcept;

private:
  struct _impl;
  std::unique_ptr<_impl> _pimpl;
};

}  // namespace utils

#endif  // CONTEXT_HPP