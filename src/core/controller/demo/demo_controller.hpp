/******************************************************************************
 *
 * @file       demo_controller.hpp
 * @brief      模板 controller 层
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef DEMO_CONTROLLER_HPP
#define DEMO_CONTROLLER_HPP

#include <core/CoreExport.hpp>
#include <core/domain/dto/demo/demo_dto.hpp>
#include <core/domain/query/demo/demo_query.hpp>
#include <memory>
#include <utils/common/type.hpp>

namespace core
{

class CORE_EXPORT DemoController
{
public:
  DemoController();
  ~DemoController();

  static DemoController& GetInstance();

  void HandleDemoGetRequest(const GetTestQuery& query, core::CommonVO& common_vo) const;
  void HandleDemoPostRequest(const PostTestDTO& dto, core::CommonVO& common_vo) const;
  void HandleDemoPutRequest(const PutTestDTO& dto, core::CommonVO& common_vo) const;
  void HandleDemoDeleteRequest(const DeleteTestQuery& query, core::CommonVO& common_vo) const;

  DemoController(const DemoController&) = delete;
  DemoController& operator=(const DemoController&) = delete;
  DemoController(DemoController&&) = delete;
  DemoController& operator=(DemoController&&) = delete;

private:
  struct _impl;
  std::unique_ptr<_impl> _pimpl;
};

}  // namespace core

#endif  // DEMO_CONTROLLER_HPP