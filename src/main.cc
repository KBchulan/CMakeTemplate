#include <tools/Logger.hpp>

// 如果有单例实现，可以在此处初始化
void init()
{
  logger;
}

int main()
{
  init();

  logger.info("Hello, {}", "World");
}