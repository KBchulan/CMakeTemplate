/******************************************************************************
 *
 * @file       type.hpp
 * @brief      类型定义
 *
 * @author     KBchulan
 * @date       2025/12/02
 * @history
 ******************************************************************************/

#ifndef TYPE_HPP
#define TYPE_HPP

#include <core/domain/vo/common_vo.hpp>
#include <expected>
#include <functional>
#include <string>
#include <utils/url/url.hpp>

using RequestHandleResult = std::expected<std::string, std::string>;
using GetHandlers = std::function<void(const utils::ParsedUrl&, core::CommonVO&)>;
using PostHandlers = std::function<void(const utils::ParsedUrl&, const std::string&, core::CommonVO&)>;
using PutHandlers = std::function<void(const utils::ParsedUrl&, const std::string&, core::CommonVO&)>;
using DeleteHandlers = std::function<void(const utils::ParsedUrl&, core::CommonVO&)>;

#endif  // TYPE_HPP