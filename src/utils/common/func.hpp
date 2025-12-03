/******************************************************************************
 *
 * @file       func.hpp
 * @brief      一些常规的工具函数
 *
 * @author     KBchulan
 * @date       2025/12/03
 * @history
 ******************************************************************************/

#ifndef FUNC_HPP
#define FUNC_HPP

#include <json/json.h>

#include <optional>
#include <sstream>
#include <string>

namespace utils
{

inline std::optional<Json::Value> ParseJson(const std::string& json_str)
{
  Json::Value root;
  Json::CharReaderBuilder builder;
  std::string errors;
  std::istringstream stream(json_str);

  if (!Json::parseFromStream(builder, stream, &root, &errors))
  {
    return std::nullopt;
  }
  return root;
}

}  // namespace utils

#endif  // FUNC_HPP