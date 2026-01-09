/******************************************************************************
 *
 * @file       common_vo.hpp
 * @brief      通用返回 vo
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef COMMON_VO_HPP
#define COMMON_VO_HPP

#include <json/json.h>

#include <cstdint>
#include <string>

namespace core
{

struct CommonVO
{
  std::int16_t code{0};
  std::string message;
  Json::Value data;

  [[nodiscard]] std::string ToString() const
  {
    Json::Value root;
    root["code"] = code;
    root["message"] = message;
    root["data"] = data;

    Json::StreamWriterBuilder writer;
    return Json::writeString(writer, root);
  }
};

}  // namespace core

#endif  // COMMON_VO_HPP