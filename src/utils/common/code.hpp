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

constexpr std::int16_t SUCCESS = 0;                 // 成功
constexpr std::int16_t URL_NOT_FOUND = 1;           // URL未找到
constexpr std::int16_t UUID_GENERATION_FAILED = 2;  // UUID生成失败
constexpr std::int16_t INVALID_PARAMETERS = 3;      // 参数无效
constexpr std::int16_t JSON_PARSE_ERROR = 4;        // JSON解析错误

}  // namespace utils

#endif  // CODE_HPP