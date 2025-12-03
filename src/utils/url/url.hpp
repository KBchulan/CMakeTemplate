/******************************************************************************
 *
 * @file       url.hpp
 * @brief      url 解析使用结构
 *
 * @author     KBchulan
 * @date       2025/12/03
 * @history
 ******************************************************************************/

#ifndef URL_HPP
#define URL_HPP

#include <boost/url.hpp>
#include <optional>
#include <string>
#include <utils/UtilsExport.hpp>

namespace utils
{

struct UTILS_EXPORT ParsedUrl
{
  std::string path;
  boost::urls::params_view params;

  // 获取单个参数
  [[nodiscard]] std::optional<std::string> Get(std::string_view key) const;

  // 获取参数，带默认值
  [[nodiscard]] std::string GetOr(std::string_view key, std::string_view default_value) const;
};

// 解析 target
std::optional<ParsedUrl> ParseUrl(std::string_view target);

}  // namespace utils

#endif  // URL_HPP