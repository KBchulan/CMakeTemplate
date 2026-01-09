#include <boost/asio/signal_set.hpp>
#include <core/business/business.hpp>
#include <core/io/io.hpp>
#include <core/server/server.hpp>
#include <global/Global.hpp>
#include <iostream>
#include <tools/Cmd.hpp>
#include <tools/Logger.hpp>
#include <utils/pool/mariadb/db_pool.hpp>
#include <utils/pool/redis/redis_pool.hpp>

using namespace global::server;

// 初始化组件
void init_components()
{
  utils::DBConfig db_config{.host = DB_HOST,
                            .port = DB_PORT,
                            .user = DB_USER,
                            .password = DB_PASSWORD,
                            .database = DB_NAME,
                            .pool_size = DB_MAX_POOL_SIZE};

  utils::RedisConfig redis_config{.host = REDIS_HOST,
                                  .port = REDIS_PORT,
                                  .password = REDIS_PASSWORD,
                                  .db_index = REDIS_DB_INDEX,
                                  .pool_size = REDIS_MAX_POOL_SIZE,
                                  .timeout = std::chrono::seconds(REDIS_TIMEOUT)};

  tools::Logger::getInstance();
  utils::DBPool::GetInstance().Init(db_config);
  utils::RedisPool::GetInstance().Init(redis_config);
  core::Business::GetInstance();
  core::IO::GetInstance();
}

// 打印使用说明
void print_usage(const char* program_name)
{
  std::cout << "Usage: " << program_name << " [-h] [-p <port>]\n"
            << "Options:\n"
            << "  -h, --help         Show this help message\n"
            << "  -p, --port <port>  Server port (default: " << DEFAULT_SERVER_PORT << ")\n";
}

// 解析命令行参数
std::optional<tools::CmdOptions> parse_cmd(std::span<char*> args)
{
  tools::CmdOptions options;

  for (std::size_t i = 1; i < args.size(); ++i)
  {
    if (std::strcmp(args[i], "-h") == 0 || std::strcmp(args[i], "--help") == 0)
    {
      options.show_help = true;
      return options;
    }

    if (std::strcmp(args[i], "-p") == 0 || std::strcmp(args[i], "--port") == 0)
    {
      if (i + 1 >= args.size())
      {
        std::cerr << "Error: missing port value\n";
        return std::nullopt;
      }

      try
      {
        int port = std::stoi(args[++i]);
        if (port < 1 || port > 65535)
        {
          std::cerr << "Error: port must be between 1 and 65535\n";
          return std::nullopt;
        }
        options.port = static_cast<unsigned short>(port);
      }
      catch (const std::exception&)
      {
        std::cerr << "Error: invalid port number '" << args[i] << "'\n";
        return std::nullopt;
      }
    }
  }

  return options;
}

int main(int argc, char* argv[])
{
  auto options = parse_cmd(std::span(argv, static_cast<std::size_t>(argc)));

  if (!options)
  {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (options->show_help)
  {
    print_usage(argv[0]);
    return EXIT_SUCCESS;
  }

  try
  {
    init_components();

    boost::asio::io_context signal_ioc;
    const auto& logger = tools::Logger::getInstance();

    boost::asio::signal_set signals(signal_ioc, SIGINT, SIGTERM);
    auto signals_handler = [&signal_ioc, &logger](const boost::system::error_code& err, int signal) -> void
    {
      if (!err)
      {
        std::cout << '\n';
        logger.info("Received signal {}, stopping http server", signal);
        signal_ioc.stop();
      }
      else
      {
        logger.error("Error receiving signal: {}", err.message());
      }
    };
    signals.async_wait(std::move(signals_handler));

    core::Server server(options->port);
    signal_ioc.run();
  }
  catch (const boost::system::system_error& e)
  {
    tools::Logger::getInstance().error("Boost system error: {}", e.what());
    return EXIT_FAILURE;
  }
  catch (const std::exception& e)
  {
    tools::Logger::getInstance().error("Standard exception: {}", e.what());
    return EXIT_FAILURE;
  }
  catch (...)
  {
    tools::Logger::getInstance().error("Unknown exception occurred");
    return EXIT_FAILURE;
  }
}