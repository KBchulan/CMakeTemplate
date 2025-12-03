/******************************************************************************
 *
 * @file       demo_dto.hpp
 * @brief      demo 模块的 dto 负载
 *
 * @author     KBchulan
 * @date       2025/12/03
 * @history
 ******************************************************************************/

#ifndef DEMO_DTO_HPP
#define DEMO_DTO_HPP

#include <core/CoreExport.hpp>
#include <optional>
#include <string>
#include <utils/common/func.hpp>

namespace core
{

struct CORE_EXPORT PostTestDTO
{
  std::string title;
  std::string content;

  static std::optional<PostTestDTO> FromJsonString(const std::string& json_str)
  {
    auto root = utils::ParseJson(json_str);
    if (!root)
    {
      return std::nullopt;
    }

    return PostTestDTO{.title = (*root)["title"].asString(), .content = (*root)["content"].asString()};
  }
};

struct CORE_EXPORT PutTestDTO
{
  std::string title;
  std::string content;

  static std::optional<PutTestDTO> FromJsonString(const std::string& json_str)
  {
    auto root = utils::ParseJson(json_str);
    if (!root)
    {
      return std::nullopt;
    }

    return PutTestDTO{.title = (*root)["title"].asString(), .content = (*root)["content"].asString()};
  }
};

}  // namespace core

#endif  // DEMO_DTO_HPP