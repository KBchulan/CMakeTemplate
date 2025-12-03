/******************************************************************************
 *
 * @file       demo_vo.hpp
 * @brief      demo 模块的 vo
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef DEMO_VO_HPP
#define DEMO_VO_HPP

#include <json/value.h>

#include <core/CoreExport.hpp>
#include <string>

namespace core
{

struct CORE_EXPORT GetTestVO
{
  std::string name;
  int age{0};

  [[nodiscard]] Json::Value ToJson() const
  {
    Json::Value root;
    root["name"] = name;
    root["age"] = age;
    return root;
  }
};

struct CORE_EXPORT PostTestVO
{
  std::string title;
  std::string content;

  [[nodiscard]] Json::Value ToJson() const
  {
    Json::Value root;
    root["title"] = title;
    root["content"] = content;
    return root;
  }
};

struct CORE_EXPORT PutTestVO
{
  std::string title;
  std::string content;

  [[nodiscard]] Json::Value ToJson() const
  {
    Json::Value root;
    root["title"] = title;
    root["content"] = content;
    return root;
  }
};

struct CORE_EXPORT DeleteTestVO
{
  std::string id;

  [[nodiscard]] Json::Value ToJson() const
  {
    Json::Value root;
    root["id"] = id;
    return root;
  }
};

}  // namespace core

#endif  // DEMO_VO_HPP