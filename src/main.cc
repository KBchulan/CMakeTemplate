#include <boost/asio/signal_set.hpp>
#include <core/server/server.hpp>
#include <global/Global.hpp>
#include <tools/Logger.hpp>

int main()
{
  try
  {
    boost::asio::io_context ioc;
    const auto& logger = tools::Logger::getInstance();

    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    auto signals_handler = [&ioc, &logger](const boost::system::error_code& err, int signal) -> void
    {
      if (!err)
      {
        logger.info("Received signal {}, stopping server", signal);
        ioc.stop();
      }
      else
      {
        logger.error("Error receiving signal: {}", err.message());
      }
    };
    signals.async_wait(std::move(signals_handler));

    core::Server server(ioc, global::server::DEFAULT_SERVER_PORT);
    ioc.run();
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