/******************************************************************************
 *
 * @file       demo_query.hpp
 * @brief      demo 模块的 query 请求
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef DEMO_QUERY_HPP
#define DEMO_QUERY_HPP

#include <charconv>
#include <core/CoreExport.hpp>
#include <string>
#include <utils/url/url.hpp>

namespace core
{

struct CORE_EXPORT GetTestQuery
{
  std::string name;
  int age{0};

  static GetTestQuery FromParsedUrl(const utils::ParsedUrl& url)
  {
    int age = 0;
    if (auto age_str = url.Get("age"))
    {
      std::from_chars(age_str->data(), age_str->data() + age_str->size(), age);
    }
    return GetTestQuery{.name = url.GetOr("name", ""), .age = age};
  }
};

struct CORE_EXPORT DeleteTestQuery
{
  std::string id;

  static DeleteTestQuery FromParsedUrl(const utils::ParsedUrl& url)
  {
    return DeleteTestQuery{.id = url.GetOr("id", "")};
  }
};

}  // namespace core

#endif  // DEMO_QUERY_HPP