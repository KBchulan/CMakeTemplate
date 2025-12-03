#include "demo_controller.hpp"

#include <core/domain/dto/demo/demo_dto.hpp>
#include <core/domain/vo/demo/demo_vo.hpp>
#include <string>
#include <tools/Id.hpp>
#include <utils/common/code.hpp>
#include <utils/common/type.hpp>

namespace core
{

struct DemoController::_impl
{
  void handle_demo_get_request(const GetTestQuery& query, core::CommonVO& common_vo) const
  {
    (void)this;

    // 校验参数
    if (query.name.empty() || query.age == 0)
    {
      common_vo.code = utils::INVALID_PARAMETERS;
      common_vo.message = "Invalid parameters";
      common_vo.data = "";
      return;
    }

    // 此处简单返回
    core::GetTestVO data;
    data.name = query.name;
    data.age = query.age;

    common_vo.code = utils::SUCCESS;
    common_vo.message = "Success";
    common_vo.data = data.ToJson();
  }

  void handle_demo_post_request(const PostTestDTO& dto, core::CommonVO& common_vo) const
  {
    (void)this;

    // 校验参数
    if (dto.title.empty() || dto.content.empty())
    {
      common_vo.code = utils::INVALID_PARAMETERS;
      common_vo.message = "Invalid parameters";
      common_vo.data = "";
      return;
    }

    // 原样返回即可
    core::PostTestVO data;
    data.title = dto.title;
    data.content = dto.content;

    common_vo.code = utils::SUCCESS;
    common_vo.message = "Success";
    common_vo.data = data.ToJson();
  }

  void handle_demo_put_request(const PutTestDTO& dto, core::CommonVO& common_vo) const
  {
    (void)this;

    // 校验参数
    if (dto.title.empty() || dto.content.empty())
    {
      common_vo.code = utils::INVALID_PARAMETERS;
      common_vo.message = "Invalid parameters";
      common_vo.data = "";
      return;
    }

    // 原样返回即可
    core::PutTestVO data;
    data.title = dto.title;
    data.content = dto.content;

    common_vo.code = utils::SUCCESS;
    common_vo.message = "Success";
    common_vo.data = data.ToJson();
  }

  void handle_demo_delete_request(const DeleteTestQuery& query, core::CommonVO& common_vo) const
  {
    (void)this;
    // 校验参数
    if (query.id.empty())
    {
      common_vo.code = utils::INVALID_PARAMETERS;
      common_vo.message = "Invalid parameters";
      common_vo.data = "";
      return;
    }

    // 给返回的 id 设置为新生成的 UUID
    core::DeleteTestVO data;

    auto result = tools::UuidGenerator::generateUuid();
    if (!result.has_value())
    {
      common_vo.code = utils::UUID_GENERATION_FAILED;
      common_vo.message = "UUID generation failed";
      common_vo.data = "";
      return;
    }
    data.id = result.value();

    common_vo.code = utils::SUCCESS;
    common_vo.message = std::string("receive id is: ") + query.id;
    common_vo.data = data.ToJson();
  }

  _impl() = default;
  ~_impl() = default;
};

DemoController::DemoController() : _pimpl(std::make_unique<_impl>())
{
}

DemoController::~DemoController() = default;

DemoController& DemoController::GetInstance()
{
  static DemoController instance;
  return instance;
}

void DemoController::HandleDemoGetRequest(const GetTestQuery& query, core::CommonVO& common_vo) const
{
  _pimpl->handle_demo_get_request(query, common_vo);
}

void DemoController::HandleDemoPostRequest(const PostTestDTO& dto, core::CommonVO& common_vo) const
{
  _pimpl->handle_demo_post_request(dto, common_vo);
}

void DemoController::HandleDemoPutRequest(const PutTestDTO& dto, core::CommonVO& common_vo) const
{
  _pimpl->handle_demo_put_request(dto, common_vo);
}

void DemoController::HandleDemoDeleteRequest(const DeleteTestQuery& query, core::CommonVO& common_vo) const
{
  _pimpl->handle_demo_delete_request(query, common_vo);
}

}  // namespace core