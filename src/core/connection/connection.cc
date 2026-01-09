#include "connection.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <core/business/business.hpp>
#include <core/logic/logic.hpp>
#include <global/Global.hpp>
#include <tools/Defer.hpp>
#include <tools/Id.hpp>
#include <tools/Logger.hpp>
#include <utils/common/jwt.hpp>
#include <utils/common/rate_limit.hpp>
#include <utils/common/routes.hpp>
#include <utils/common/type.hpp>
#include <utils/context/context.hpp>

namespace core
{

struct Connection::_impl
{
  boost::asio::ip::tcp::socket _socket;
  boost::beast::flat_buffer _buffer{global::server::MAX_FLATBUFFER_SIZE};
  boost::beast::http::request<boost::beast::http::dynamic_body> _request;
  boost::beast::http::response<boost::beast::http::dynamic_body> _response;
  boost::asio::steady_timer _deadline;

  void do_read(const std::shared_ptr<Connection>& self)
  {
    boost::beast::http::async_read(_socket, _buffer, _request,
                                   boost::beast::bind_front_handler(&_impl::on_read, this, self));
  }

  void on_read(const std::shared_ptr<Connection>& self, const boost::beast::error_code& errc,
               [[maybe_unused]] std::size_t bytes_transferred)
  {
    if (errc)
    {
      // 读取请求出错，记录日志并关闭连接
      if (errc == boost::beast::http::error::end_of_stream || errc == boost::asio::error::connection_reset ||
          errc == boost::asio::error::broken_pipe)
      {
        tools::Logger::getInstance().debug("Client closed connection");
      }
      else
      {
        tools::Logger::getInstance().error("Read request error, error msg: {}", errc.message());
      }
      return;
    }

    // 成功读取请求，处理请求并启动定时器
    _deadline.expires_after(std::chrono::seconds(60));
    _deadline.async_wait(boost::beast::bind_front_handler(&_impl::on_deadline, this, self));
    handle_request(self);
  }

  void on_write(const std::shared_ptr<Connection>& self, const boost::beast::error_code& errc,
                [[maybe_unused]] std::size_t bytes_transferred)
  {
    if (errc)
    {
      tools::Logger::getInstance().error("Send response error, error msg: {}", errc.message());
      return;
    }

    if (_response.need_eof())
    {
      boost::beast::error_code shutdown_ec;
      _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, shutdown_ec);
      _deadline.cancel();
    }
    else
    {
      _request = {};
      _response = {};
      _deadline.cancel();
      do_read(self);
    }
  }

  void on_deadline([[maybe_unused]] const std::shared_ptr<Connection>& self, boost::system::error_code errc)
  {
    if (!errc)
    {
      boost::beast::error_code shutdown_ec;
      _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, shutdown_ec);
    }
  }

  void handle_request(const std::shared_ptr<Connection>& self)
  {
    // 处理健康检查请求
    if (_request.target() == utils::HEALTH_CHECK_ROUTE && _request.method() == boost::beast::http::verb::get)
    {
      _response.result(boost::beast::http::status::ok);
      boost::beast::ostream(_response.body()) << R"({"status": "ok"})";
      send_response(self);
      return;
    }

    // 获取 logger 实例
    const auto& logger = tools::Logger::getInstance();

    // 构造响应的基本信息
    _response.version(_request.version());
    _response.keep_alive(_request.keep_alive());
    _response.set(boost::beast::http::field::content_type, "application/json");
    _response.set(boost::beast::http::field::server, "CMakeTemplate-HttpServer");

    // 限流校验
    auto client_ip = _socket.remote_endpoint().address().to_string();
    if (!utils::RateLimit::Allow(client_ip))
    {
      int status_code = static_cast<int>(boost::beast::http::status::too_many_requests);
      logger.error("| {} | {} | {} | {}", client_ip, _request.method_string(), status_code, _request.target());
      _response.result(boost::beast::http::status::too_many_requests);
      _response.set(boost::beast::http::field::retry_after, "60");
      boost::beast::ostream(_response.body()) << R"({"code": 429, "message": "Too Many Requests"})";
      send_response(self);
      return;
    }

    // 看需求增加幂等校验机制，暂不需要

    // 解析 URL
    auto url = utils::ParseUrl(_request.target());
    if (!url.has_value())
    {
      int status_code = static_cast<int>(boost::beast::http::status::internal_server_error);
      logger.error("| {} | {} | {} | {}", client_ip, _request.method_string(), status_code, _request.target());
      _response.result(boost::beast::http::status::internal_server_error);
      boost::beast::ostream(_response.body()) << R"({"code": 500, "message": "Invalid URL"})";
      send_response(self);
      return;
    }

    // 判断是否支持服务，用于服务降级使用
    if (!utils::AVAILABLE_ROUTES.contains(url->path))
    {
      int status_code = static_cast<int>(boost::beast::http::status::method_not_allowed);
      logger.error("| {} | {} | {} | {}", client_ip, _request.method_string(), status_code, _request.target());
      _response.result(boost::beast::http::status::method_not_allowed);
      boost::beast::ostream(_response.body()) << R"({"code": 405, "message": "this request is not allowed"})";
      send_response(self);
      return;
    }

    // 构造请求上下文
    std::string body = boost::beast::buffers_to_string(_request.body().data());
    utils::Context ctx{url.value(), body};

    // 增加 request id 方便追踪日志
    auto request_id = tools::UuidGenerator::generateUuid();
    if (!request_id.has_value())
    {
      int status_code = static_cast<int>(boost::beast::http::status::internal_server_error);
      logger.error("| {} | {} | {} | {}", client_ip, _request.method_string(), status_code, _request.target());
      _response.result(boost::beast::http::status::internal_server_error);
      boost::beast::ostream(_response.body()) << R"({"code": 500, "message": "Generate request ID failed"})";
      send_response(self);
      return;
    }

    ctx.Set("request_id", request_id.value());
    _response.set("X-Request-ID", request_id.value());

    // 增加 JWT 校验机制
    if (!utils::NO_AUTH_ROUTES.contains(ctx.GetUrl().path))
    {
      auto iter = _request.find(boost::beast::http::field::authorization);
      if (iter == _request.end())
      {
        int status_code = static_cast<int>(boost::beast::http::status::unauthorized);
        logger.error("| {} | {} | {} | {} | {}", client_ip, request_id.value(), _request.method_string(), status_code,
                     _request.target());

        _response.result(boost::beast::http::status::unauthorized);
        boost::beast::ostream(_response.body()) << R"({"code": 401, "message": "Missing Authorization Header"})";
        send_response(self);
        return;
      }

      // 校验 JWT
      std::string auth_header{iter->value()};
      if (auth_header.size() <= 7 || !auth_header.starts_with("Bearer "))
      {
        int status_code = static_cast<int>(boost::beast::http::status::unauthorized);
        logger.error("| {} | {} | {} | {} | {}", client_ip, request_id.value(), _request.method_string(), status_code,
                     _request.target());

        _response.result(boost::beast::http::status::unauthorized);
        boost::beast::ostream(_response.body()) << R"({"code": 401, "message": "Invalid Authorization Format"})";
        send_response(self);
        return;
      }

      auto jwt_token = auth_header.substr(7);

      // 验证 token
      auto payload = utils::Jwt::ParseToken(jwt_token);
      if (!payload.has_value())
      {
        int status_code = static_cast<int>(boost::beast::http::status::unauthorized);
        logger.error("| {} | {} | {} | {} | {}", client_ip, request_id.value(), _request.method_string(), status_code,
                     _request.target());

        _response.result(boost::beast::http::status::unauthorized);
        boost::beast::ostream(_response.body()) << R"({"code": 401, "message": "Invalid or Expired Token"})";
        send_response(self);
        return;
      }

      // 将解析出的用户信息注入上下文，供后续业务逻辑使用
      ctx.Set("user_uuid", payload->uuid);
    }

    // 将请求交给业务线程池处理
    boost::asio::post(
        Business::GetInstance().GetBusinessPool(),
        [this, self, ctx = std::move(ctx), client_ip = std::move(client_ip), request_id = request_id.value(),
         method = _request.method(), method_str = std::string(_request.method_string()),
         target = std::string(_request.target())]() mutable
        {
          // 处理请求
          auto result = [&]() -> RequestHandleResult
          {
            switch (method)
            {
              case boost::beast::http::verb::get:
                return Logic::GetInstance().HandleGetRequest(ctx);
              case boost::beast::http::verb::post:
                return Logic::GetInstance().HandlePostRequest(ctx);
              case boost::beast::http::verb::put:
                return Logic::GetInstance().HandlePutRequest(ctx);
              case boost::beast::http::verb::delete_:
                return Logic::GetInstance().HandleDeleteRequest(ctx);
              default:
                return std::unexpected("Unsupported HTTP method");
            }
          }();

          // 将结果发送回客户端，重新回到网络线程
          boost::asio::post(_socket.get_executor(),
                            [this, self, result = std::move(result), client_ip = std::move(client_ip),
                             request_id = std::move(request_id), method_str = std::move(method_str),
                             target = std::move(target)]() mutable
                            {
                              int status_code = 0;

                              if (result.has_value())
                              {
                                status_code = static_cast<int>(boost::beast::http::status::ok);
                                tools::Logger::getInstance().info("| {} | {} | {} | {} | {}", client_ip, request_id,
                                                                  method_str, status_code, target);

                                _response.result(boost::beast::http::status::ok);
                                boost::beast::ostream(_response.body()) << result.value();
                              }
                              else
                              {
                                status_code = static_cast<int>(boost::beast::http::status::bad_request);
                                tools::Logger::getInstance().error("| {} | {} | {} | {} | {}", client_ip, request_id,
                                                                   method_str, status_code, target);

                                _response.result(boost::beast::http::status::bad_request);
                                boost::beast::ostream(_response.body()) << result.error();
                              }

                              send_response(self);
                            });
        });
  }

  void send_response(const std::shared_ptr<Connection>& self)
  {
    _response.prepare_payload();
    boost::beast::http::async_write(_socket, _response, boost::beast::bind_front_handler(&_impl::on_write, this, self));
  }

  explicit _impl(boost::asio::ip::tcp::socket socket) : _socket(std::move(socket)), _deadline(_socket.get_executor())
  {
  }
};

Connection::Connection(boost::asio::ip::tcp::socket socket) : _pimpl(std::make_unique<_impl>(std::move(socket)))
{
}

Connection::~Connection() = default;

void Connection::Start()
{
  _pimpl->do_read(shared_from_this());
}

}  // namespace core