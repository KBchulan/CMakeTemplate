/******************************************************************************
 *
 * @file       db_pool.hpp
 * @brief      MariaDB/Mysql 的连接池子
 *
 * @author     KBchulan
 * @date       2025/12/04
 * @history
 ******************************************************************************/

#ifndef DB_POOL_HPP
#define DB_POOL_HPP

#include <atomic>
#include <cstring>
#include <global/Global.hpp>
#include <utils/db_params/db_params.hpp>

namespace utils
{

struct UTILS_EXPORT DBConfig
{
  std::string host;
  std::uint16_t port;
  std::string user;
  std::string password;
  std::string database;
  std::size_t pool_size;
};

class DBPool;
class UTILS_EXPORT PooledConnection
{
public:
  PooledConnection(MYSQL* conn, DBPool* pool, std::size_t slot);

  ~PooledConnection();

  [[nodiscard]] MYSQL* GetConnection() const;

  // 执行 SQL 语句 (INSERT/UPDATE/DELETE)
  bool Execute(const char* sql, const std::vector<ParamHolder>& params);

  // 查询单行 (SELECT ... LIMIT 1)，成功返回 true
  bool QueryOne(const char* sql, const std::vector<ParamHolder>& params, const std::vector<ResultHolder>& results);

  // 查询多行，返回行数
  template <typename Callback>
  std::size_t QueryMany(const char* sql, const std::vector<ParamHolder>& params,
                        const std::vector<ResultHolder>& results, Callback callback);

  PooledConnection(const PooledConnection&) = delete;
  PooledConnection& operator=(const PooledConnection&) = delete;
  PooledConnection(PooledConnection&& other) noexcept;
  PooledConnection& operator=(PooledConnection&&) noexcept = delete;

private:
  MYSQL* _conn;
  DBPool* _pool;
  std::size_t _slot;
};

class UTILS_EXPORT DBPool
{
  static constexpr std::size_t MAX_POOL_SIZE = global::server::DB_MAX_POOL_SIZE;

  struct Slot
  {
    MYSQL* _conn = nullptr;
    std::atomic<bool> _in_use{false};
  };

public:
  static DBPool& GetInstance();

  void Init(const DBConfig& config);

  [[nodiscard]] PooledConnection GetConnection();

  void ReleaseConnection(std::size_t slot);

private:
  DBPool();
  ~DBPool();

  [[nodiscard]] MYSQL* create_connection() const;

  DBConfig _config{};
  std::size_t _pool_size{0};
  std::array<Slot, MAX_POOL_SIZE> _slots;
  std::atomic<std::size_t> _available{0};
};

}  // namespace utils

#endif  // DB_POOL_HPP