/******************************************************************************
 *
 * @file       func.hpp
 * @brief      一些常规的工具函数
 *
 * @author     KBchulan
 * @date       2025/12/03
 * @history
 ******************************************************************************/

#ifndef FUNC_HPP
#define FUNC_HPP

#include <json/json.h>
#include <sodium.h>
#include <sodium/core.h>
#include <sodium/crypto_pwhash.h>

#include <array>
#include <expected>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>

namespace utils
{

inline std::optional<Json::Value> ParseJson(const std::string& json_str)
{
  Json::Value root;
  Json::CharReaderBuilder builder;
  std::string errors;
  std::istringstream stream(json_str);

  if (!Json::parseFromStream(builder, stream, &root, &errors))
  {
    return std::nullopt;
  }
  return root;
}

inline std::expected<std::string, std::string> HashPassWord(const std::string& password)
{
  static std::once_flag flag;
  static bool init_success = false;

  std::call_once(flag, []() { init_success = (sodium_init() >= 0); });

  if (!init_success)
  {
    return std::unexpected("libsodium init failed");
  }

  std::array<char, crypto_pwhash_STRBYTES> hashed;
  if (crypto_pwhash_str(hashed.data(), password.c_str(), password.length(), crypto_pwhash_OPSLIMIT_MODERATE,
                        crypto_pwhash_MEMLIMIT_MODERATE) != 0)
  {
    return std::unexpected("out of memory");
  }

  return std::string(hashed.data());
}

inline bool VerifyPassword(const std::string& password, const std::string& hash)
{
  return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.length()) == 0;
}

}  // namespace utils

#endif  // FUNC_HPP