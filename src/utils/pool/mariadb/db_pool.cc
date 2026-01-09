#include "db_pool.hpp"

#include <algorithm>
#include <tools/Logger.hpp>

namespace utils
{

PooledConnection::PooledConnection(MYSQL* conn, DBPool* pool, std::size_t slot) : _conn(conn), _pool(pool), _slot(slot)
{
}

PooledConnection::~PooledConnection()
{
  if (_conn != nullptr && _pool != nullptr)
  {
    _pool->ReleaseConnection(_slot);
  }
}

MYSQL* PooledConnection::GetConnection() const
{
  return _conn;
}

bool PooledConnection::Execute(const char* sql, const std::vector<ParamHolder>& params)
{
  MYSQL_STMT* stmt = mysql_stmt_init(_conn);
  if (stmt == nullptr)
  {
    tools::Logger::getInstance().error("mysql_stmt_init failed: {}", mysql_error(_conn));
    return false;
  }

  if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0)
  {
    tools::Logger::getInstance().error("mysql_stmt_prepare failed: {}", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  // 提取 MYSQL_BIND 数组
  if (!params.empty())
  {
    std::vector<MYSQL_BIND> binds(params.size());
    std::ranges::transform(params, binds.begin(), [](const ParamHolder& holder) { return holder.bind; });

    if (mysql_stmt_bind_param(stmt, binds.data()) != 0)
    {
      tools::Logger::getInstance().error("mysql_stmt_bind_param failed: {}", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return false;
    }
  }

  int result = mysql_stmt_execute(stmt);
  if (result != 0)
  {
    tools::Logger::getInstance().error("mysql_stmt_execute failed: {}", mysql_stmt_error(stmt));
  }
  mysql_stmt_close(stmt);
  return result == 0;
}

bool PooledConnection::QueryOne(const char* sql, const std::vector<ParamHolder>& params,
                                const std::vector<ResultHolder>& results)
{
  MYSQL_STMT* stmt = mysql_stmt_init(_conn);
  if (stmt == nullptr)
  {
    tools::Logger::getInstance().error("mysql_stmt_init failed: {}", mysql_error(_conn));
    return false;
  }

  if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0)
  {
    tools::Logger::getInstance().error("mysql_stmt_prepare failed: {}", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  // 绑定输入参数
  if (!params.empty())
  {
    std::vector<MYSQL_BIND> binds(params.size());
    std::ranges::transform(params, binds.begin(), [](const ParamHolder& holder) { return holder.bind; });

    if (mysql_stmt_bind_param(stmt, binds.data()) != 0)
    {
      tools::Logger::getInstance().error("mysql_stmt_bind_param failed: {}", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return false;
    }
  }

  if (mysql_stmt_execute(stmt) != 0)
  {
    tools::Logger::getInstance().error("mysql_stmt_execute failed: {}", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  // 绑定输出结果
  if (!results.empty())
  {
    std::vector<MYSQL_BIND> binds(results.size());
    std::ranges::transform(results, binds.begin(), [](const ResultHolder& holder) { return holder.bind; });

    if (mysql_stmt_bind_result(stmt, binds.data()) != 0)
    {
      tools::Logger::getInstance().error("mysql_stmt_bind_result failed: {}", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return false;
    }
  }

  bool success = (mysql_stmt_fetch(stmt) == 0);
  mysql_stmt_close(stmt);
  return success;
}

template <typename Callback>
std::size_t PooledConnection::QueryMany(const char* sql, const std::vector<ParamHolder>& params,
                                        const std::vector<ResultHolder>& results, Callback callback)
{
  MYSQL_STMT* stmt = mysql_stmt_init(_conn);
  if (stmt == nullptr)
  {
    tools::Logger::getInstance().error("mysql_stmt_init failed: {}", mysql_error(_conn));
    return 0;
  }

  if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0)
  {
    tools::Logger::getInstance().error("mysql_stmt_prepare failed: {}", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return 0;
  }

  // 绑定输入参数
  if (!params.empty())
  {
    std::vector<MYSQL_BIND> binds(params.size());
    std::ranges::transform(params, binds.begin(), [](const ParamHolder& holder) { return holder.bind; });

    if (mysql_stmt_bind_param(stmt, binds.data()) != 0)
    {
      tools::Logger::getInstance().error("mysql_stmt_bind_param failed: {}", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return 0;
    }
  }

  if (mysql_stmt_execute(stmt) != 0)
  {
    tools::Logger::getInstance().error("mysql_stmt_execute failed: {}", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return 0;
  }

  // 绑定输出结果
  if (!results.empty())
  {
    std::vector<MYSQL_BIND> binds(results.size());
    std::ranges::transform(results, binds.begin(), [](const ResultHolder& holder) { return holder.bind; });

    if (mysql_stmt_bind_result(stmt, binds.data()) != 0)
    {
      tools::Logger::getInstance().error("mysql_stmt_bind_result failed: {}", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return 0;
    }
  }

  std::size_t count = 0;
  while (mysql_stmt_fetch(stmt) == 0)
  {
    callback();
    ++count;
  }

  mysql_stmt_close(stmt);
  return count;
}

PooledConnection::PooledConnection(PooledConnection&& other) noexcept
    : _conn(other._conn), _pool(other._pool), _slot(other._slot)
{
  other._conn = nullptr;
}

DBPool& DBPool::GetInstance()
{
  static DBPool instance;
  return instance;
}

void DBPool::Init(const DBConfig& config)
{
  _config = config;
  _pool_size = std::min(config.pool_size, MAX_POOL_SIZE);

  for (std::size_t i = 0; i < _pool_size; ++i)
  {
    _slots[i]._conn = create_connection();
    _slots[i]._in_use.store(false, std::memory_order_relaxed);
  }
  _available.store(_pool_size, std::memory_order_release);

  tools::Logger::getInstance().info("mariadb pool init successful");
}

PooledConnection DBPool::GetConnection()
{
  while (true)
  {
    for (std::size_t i = 0; i < _pool_size; ++i)
    {
      bool expected = false;
      if (_slots[i]._in_use.compare_exchange_strong(expected, true, std::memory_order_acquire,
                                                    std::memory_order_relaxed))
      {
        _available.fetch_sub(1, std::memory_order_acquire);

        // 失效就重新连接
        if (mysql_ping(_slots[i]._conn) != 0)
        {
          mysql_close(_slots[i]._conn);
          _slots[i]._conn = create_connection();
        }

        return {_slots[i]._conn, this, i};
      }
    }

    _available.wait(0, std::memory_order_relaxed);
  }
}

void DBPool::ReleaseConnection(std::size_t slot)
{
  _slots[slot]._in_use.store(false, std::memory_order_release);
  _available.fetch_add(1, std::memory_order_release);
  _available.notify_one();
}

DBPool::DBPool() = default;

DBPool::~DBPool()
{
  for (std::size_t i = 0; i < _pool_size; ++i)
  {
    if (_slots[i]._conn != nullptr)
    {
      mysql_close(_slots[i]._conn);
    }
  }

  tools::Logger::getInstance().info("mariadb pool closed");
}

MYSQL* DBPool::create_connection() const
{
  MYSQL* conn = mysql_init(nullptr);
  if (conn == nullptr)
  {
    throw std::runtime_error("Failed to initialize MySQL connection");
  }

  unsigned int timeout = 5;
  mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

  if (mysql_real_connect(conn, _config.host.c_str(), _config.user.c_str(), _config.password.c_str(),
                         _config.database.c_str(), _config.port, nullptr, 0) == nullptr)
  {
    std::string err_msg = mysql_error(conn);
    mysql_close(conn);
    throw std::runtime_error("Failed to connect to MySQL: " + err_msg);
  }

  mysql_set_character_set(conn, "utf8mb4");
  return conn;
}

}  // namespace utils