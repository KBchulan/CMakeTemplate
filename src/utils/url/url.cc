#include "url.hpp"

namespace utils
{

std::optional<std::string> ParsedUrl::Get(std::string_view key) const
{
  if (auto iter = params.find(key); iter != params.end())
  {
    return std::string((*iter).value);
  }
  return std::nullopt;
}

std::string ParsedUrl::GetOr(std::string_view key, std::string_view default_value) const
{
  return Get(key).value_or(std::string(default_value));
}

std::optional<ParsedUrl> ParseUrl(std::string_view target)
{
  auto result = boost::urls::parse_origin_form(target);
  if (!result)
  {
    return std::nullopt;
  }

  return ParsedUrl{.path = std::string(result->path()), .params = result->params()};
}

}  // namespace utils
