#include <string>
#include <tools/Logger.hpp>

int main()
{
  auto &logger = tools::Logger::getInstance();
  logger.info("Hello, {}", "World!");
}