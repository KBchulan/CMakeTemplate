/******************************************************************************
 *
 * @file       jwt.hpp
 * @brief      JWT 组件
 *
 * @author     KBchulan
 * @date       2025/12/17
 * @history
 ******************************************************************************/

#ifndef JWT_HPP
#define JWT_HPP

#include <jwt-cpp/jwt.h>

#include <global/Global.hpp>
#include <optional>
#include <string>
#include <utils/UtilsExport.hpp>

namespace utils
{

struct UTILS_EXPORT TokenPayload
{
  std::string uuid;
};

class UTILS_EXPORT Jwt
{
public:
  [[nodiscard]] static std::string GenerateToken(const TokenPayload& payload)
  {
    using namespace global::server;

    return jwt::create()
        .set_issuer(JWT_ISSUER)
        .set_type("JWS")
        .set_payload_claim("uuid", jwt::claim(payload.uuid))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(JWT_EXPIRATION_TIME))
        .sign(jwt::algorithm::hs256{JWT_DEFAULT_SECRET});
  }

  [[nodiscard]] static std::optional<TokenPayload> ParseToken(const std::string& token)
  {
    using namespace global::server;

    try
    {
      auto decoded = jwt::decode(token);
      auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{JWT_DEFAULT_SECRET}).with_issuer(JWT_ISSUER);
      verifier.verify(decoded);

      if (decoded.has_payload_claim("uuid"))
      {
        return TokenPayload{decoded.get_payload_claim("uuid").as_string()};
      }

      return std::nullopt;
    }
    catch (const std::exception& e)
    {
      return std::nullopt;
    }
  }
};

}  // namespace utils

#endif  // JWT_HPP