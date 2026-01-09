/******************************************************************************
 *
 * @file       code.hpp
 * @brief      返回状态码集中定义
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef CODE_HPP
#define CODE_HPP

#include <cstdint>

namespace utils
{

constexpr std::int16_t SUCCESS = 0;        // 成功
constexpr std::int16_t URL_NOT_FOUND = 1;  // URL未找到

}  // namespace utils

#endif  // CODE_HPP