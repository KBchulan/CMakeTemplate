#include "logic.hpp"

#include <core/domain/vo/common_vo.hpp>
#include <tools/Logger.hpp>
#include <unordered_map>
#include <utils/common/code.hpp>
#include <utils/common/routes.hpp>

namespace core
{

struct Logic::_impl
{
  std::unordered_map<std::string, GetHandlers> _get_handlers;
  std::unordered_map<std::string, PostHandlers> _post_handlers;
  std::unordered_map<std::string, PutHandlers> _put_handlers;
  std::unordered_map<std::string, DeleteHandlers> _delete_handlers;

  RequestHandleResult handle_get_request(const utils::Context& ctx)
  {
    core::CommonVO common_vo{};

    std::string path = ctx.GetUrl().path;
    if (_get_handlers.contains(path))
    {
      _get_handlers[path](ctx, common_vo);
      if (common_vo.code == utils::SUCCESS)
      {
        return common_vo.ToString();
      }
      return std::unexpected(common_vo.ToString());
    }

    common_vo.code = utils::URL_NOT_FOUND;
    common_vo.message = "URL not found";
    common_vo.data = "";
    return std::unexpected(common_vo.ToString());
  }

  RequestHandleResult handle_post_request(const utils::Context& ctx)
  {
    core::CommonVO common_vo{};

    std::string path = ctx.GetUrl().path;
    if (_post_handlers.contains(path))
    {
      _post_handlers[path](ctx, common_vo);
      if (common_vo.code == utils::SUCCESS)
      {
        return common_vo.ToString();
      }
      return std::unexpected(common_vo.ToString());
    }

    common_vo.code = utils::URL_NOT_FOUND;
    common_vo.message = "URL not found";
    common_vo.data = "";
    return std::unexpected(common_vo.ToString());
  }

  RequestHandleResult handle_put_request(const utils::Context& ctx)
  {
    core::CommonVO common_vo{};

    std::string path = ctx.GetUrl().path;
    if (_put_handlers.contains(path))
    {
      _put_handlers[path](ctx, common_vo);
      if (common_vo.code == utils::SUCCESS)
      {
        return common_vo.ToString();
      }
      return std::unexpected(common_vo.ToString());
    }

    common_vo.code = utils::URL_NOT_FOUND;
    common_vo.message = "URL not found";
    common_vo.data = "";
    return std::unexpected(common_vo.ToString());
  }

  RequestHandleResult handle_delete_request(const utils::Context& ctx)
  {
    core::CommonVO common_vo{};

    std::string path = ctx.GetUrl().path;
    if (_delete_handlers.contains(path))
    {
      _delete_handlers[path](ctx, common_vo);
      if (common_vo.code == utils::SUCCESS)
      {
        return common_vo.ToString();
      }
      return std::unexpected(common_vo.ToString());
    }

    common_vo.code = utils::URL_NOT_FOUND;
    common_vo.message = "URL not found";
    common_vo.data = "";
    return std::unexpected(common_vo.ToString());
  }

  _impl()
  {
    routes_demo();
  }

  ~_impl() = default;

  void routes_demo()
  {
  }
};

Logic::Logic() : _pimpl(std::make_unique<_impl>())
{
}

Logic::~Logic() = default;

Logic& Logic::GetInstance()
{
  static Logic instance;
  return instance;
}

RequestHandleResult Logic::HandleGetRequest(const utils::Context& ctx)
{
  return _pimpl->handle_get_request(ctx);
}

RequestHandleResult Logic::HandlePostRequest(const utils::Context& ctx)
{
  return _pimpl->handle_post_request(ctx);
}

RequestHandleResult Logic::HandlePutRequest(const utils::Context& ctx)
{
  return _pimpl->handle_put_request(ctx);
}

RequestHandleResult Logic::HandleDeleteRequest(const utils::Context& ctx)
{
  return _pimpl->handle_delete_request(ctx);
}

}  // namespace core