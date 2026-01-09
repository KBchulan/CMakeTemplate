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

#include <string>
#include <unordered_set>

namespace utils
{

#define UTILS_API_V1_PREFIX "/api/v1"
#define UTILS_ROUTE(path) UTILS_API_V1_PREFIX path

// ================
// user 模块路由
// ================
constexpr const char* HEALTH_CHECK_ROUTE = UTILS_ROUTE("/health-check");

// 用于 JWT 校验
inline const std::unordered_set<std::string> NO_AUTH_ROUTES = {HEALTH_CHECK_ROUTE};

// 用于服务降级使用
inline const std::unordered_set<std::string> AVAILABLE_ROUTES = {HEALTH_CHECK_ROUTE};

}  // namespace utils

#endif  // ROUTES_HPP