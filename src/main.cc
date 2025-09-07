#include <tools/Logger.hpp>

int main()
{
  const auto &logger = tools::Logger::getInstance();
  logger.info("Hello, {}", "World!");
}