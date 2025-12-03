#include "logic.hpp"

#include <core/controller/demo/demo_controller.hpp>
#include <core/domain/dto/demo/demo_dto.hpp>
#include <core/domain/query/demo/demo_query.hpp>
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

  RequestHandleResult handle_get_request(const utils::ParsedUrl& url)
  {
    core::CommonVO common_vo{};

    if (_get_handlers.contains(url.path))
    {
      _get_handlers[url.path](url, common_vo);
      return common_vo.ToString();
    }

    common_vo.code = utils::URL_NOT_FOUND;
    common_vo.message = "URL not found";
    common_vo.data = "";
    return std::unexpected(common_vo.ToString());
  }

  RequestHandleResult handle_post_request(const utils::ParsedUrl& url, const std::string& body)
  {
    core::CommonVO common_vo{};

    if (_post_handlers.contains(url.path))
    {
      _post_handlers[url.path](url, body, common_vo);
      return common_vo.ToString();
    }

    common_vo.code = utils::URL_NOT_FOUND;
    common_vo.message = "URL not found";
    common_vo.data = "";
    return std::unexpected(common_vo.ToString());
  }

  RequestHandleResult handle_put_request(const utils::ParsedUrl& url, const std::string& body)
  {
    core::CommonVO common_vo{};

    if (_put_handlers.contains(url.path))
    {
      _put_handlers[url.path](url, body, common_vo);
      return common_vo.ToString();
    }

    common_vo.code = utils::URL_NOT_FOUND;
    common_vo.message = "URL not found";
    common_vo.data = "";
    return std::unexpected(common_vo.ToString());
  }

  RequestHandleResult handle_delete_request(const utils::ParsedUrl& url)
  {
    core::CommonVO common_vo{};

    if (_delete_handlers.contains(url.path))
    {
      _delete_handlers[url.path](url, common_vo);
      return common_vo.ToString();
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
    _get_handlers[utils::DEMO_GET_TEST_ROUTE] = [](const utils::ParsedUrl& url, core::CommonVO& common_vo)
    {
      auto query = GetTestQuery::FromParsedUrl(url);
      DemoController::GetInstance().HandleDemoGetRequest(query, common_vo);
    };

    _post_handlers[utils::DEMO_POST_TEST_ROUTE] =
        [](const utils::ParsedUrl&, const std::string& body, core::CommonVO& common_vo)
    {
      auto dto = PostTestDTO::FromJsonString(body);
      if (!dto.has_value())
      {
        common_vo.code = utils::JSON_PARSE_ERROR;
        common_vo.message = "JSON parse error";
        common_vo.data = "";
        return;
      }

      DemoController::GetInstance().HandleDemoPostRequest(dto.value(), common_vo);
    };

    _put_handlers[utils::DEMO_PUT_TEST_ROUTE] =
        [](const utils::ParsedUrl&, const std::string& body, core::CommonVO& common_vo)
    {
      auto dto = PutTestDTO::FromJsonString(body);
      if (!dto.has_value())
      {
        common_vo.code = utils::JSON_PARSE_ERROR;
        common_vo.message = "JSON parse error";
        common_vo.data = "";
        return;
      }

      DemoController::GetInstance().HandleDemoPutRequest(dto.value(), common_vo);
    };

    _delete_handlers[utils::DEMO_DELETE_TEST_ROUTE] = [](const utils::ParsedUrl& url, core::CommonVO& common_vo)
    {
      auto query = DeleteTestQuery::FromParsedUrl(url);
      DemoController::GetInstance().HandleDemoDeleteRequest(query, common_vo);
    };
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

RequestHandleResult Logic::HandleGetRequest(const utils::ParsedUrl& url)
{
  return _pimpl->handle_get_request(url);
}

RequestHandleResult Logic::HandlePostRequest(const utils::ParsedUrl& url, const std::string& body)
{
  return _pimpl->handle_post_request(url, body);
}

RequestHandleResult Logic::HandlePutRequest(const utils::ParsedUrl& url, const std::string& body)
{
  return _pimpl->handle_put_request(url, body);
}

RequestHandleResult Logic::HandleDeleteRequest(const utils::ParsedUrl& url)
{
  return _pimpl->handle_delete_request(url);
}

}  // namespace core