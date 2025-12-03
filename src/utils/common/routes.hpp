/******************************************************************************
 *
 * @file       routes.hpp
 * @brief      路由集中声明处
 *
 * @author     KBchulan
 * @date       2025/12/03
 * @history
 ******************************************************************************/

#ifndef ROUTES_HPP
#define ROUTES_HPP

namespace utils
{

// ================
// demo 模块路由
// ================
constexpr const char* DEMO_GET_TEST_ROUTE = "/demo/get-test";
constexpr const char* DEMO_POST_TEST_ROUTE = "/demo/post-test";
constexpr const char* DEMO_PUT_TEST_ROUTE = "/demo/put-test";
constexpr const char* DEMO_DELETE_TEST_ROUTE = "/demo/delete-test";

}  // namespace utils

#endif  // ROUTES_HPP