#include "context.hpp"

#include <unordered_map>
#include <utility>

namespace utils
{

struct Context::_impl
{
  std::unordered_map<std::string, std::any> _personal_val;
  ParsedUrl _url;
  std::string _body;

  _impl(ParsedUrl&& url, std::string&& body) : _url(std::move(url)), _body(std::move(body))
  {
  }
  ~_impl() = default;
};

Context::Context(ParsedUrl& url, std::string& body) : _pimpl(std::make_unique<_impl>(std::move(url), std::move(body)))
{
}

Context::~Context() = default;

Context::Context(Context&& other) noexcept : _pimpl(std::move(other._pimpl))
{
}

Context& Context::operator=(Context&& other) noexcept
{
  if (this != &other)
  {
    _pimpl = std::move(other._pimpl);
  }
  return *this;
}

void Context::Set(const std::string& key, std::any value)
{
  _pimpl->_personal_val[key] = std::move(value);
}

[[nodiscard]] std::any Context::Get(const std::string& key) const
{
  auto iter = _pimpl->_personal_val.find(key);
  if (iter != _pimpl->_personal_val.end())
  {
    return iter->second;
  }
  return std::any{};
}

[[nodiscard]] const ParsedUrl& Context::GetUrl() const
{
  return _pimpl->_url;
}

[[nodiscard]] const std::string& Context::GetBody() const
{
  return _pimpl->_body;
}

}  // namespace utils