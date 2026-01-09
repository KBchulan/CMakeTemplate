#include "db_params.hpp"

namespace utils
{

// ============================================================================
// 参数绑定 (用于 INSERT/UPDATE/DELETE 的输入参数)
// ============================================================================

// 字符串类型
ParamHolder MakeBind(const std::string& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_STRING;
  holder.bind.buffer = const_cast<char*>(value.data());
  holder.bind.buffer_length = value.size();
  holder.length = value.size();
  holder.bind.length = &holder.length;
  return holder;
}

// 有符号整数类型
ParamHolder MakeBind(const std::int8_t& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_TINY;
  holder.bind.buffer = const_cast<std::int8_t*>(&value);
  holder.bind.is_unsigned = 0;
  return holder;
}

ParamHolder MakeBind(const std::int16_t& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_SHORT;
  holder.bind.buffer = const_cast<std::int16_t*>(&value);
  holder.bind.is_unsigned = 0;
  return holder;
}

ParamHolder MakeBind(const std::int32_t& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_LONG;
  holder.bind.buffer = const_cast<std::int32_t*>(&value);
  holder.bind.is_unsigned = 0;
  return holder;
}

ParamHolder MakeBind(const std::int64_t& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_LONGLONG;
  holder.bind.buffer = const_cast<std::int64_t*>(&value);
  holder.bind.is_unsigned = 0;
  return holder;
}

// 无符号整数类型
ParamHolder MakeBind(const std::uint8_t& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_TINY;
  holder.bind.buffer = const_cast<std::uint8_t*>(&value);
  holder.bind.is_unsigned = 1;
  return holder;
}

ParamHolder MakeBind(const std::uint16_t& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_SHORT;
  holder.bind.buffer = const_cast<std::uint16_t*>(&value);
  holder.bind.is_unsigned = 1;
  return holder;
}

ParamHolder MakeBind(const std::uint32_t& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_LONG;
  holder.bind.buffer = const_cast<std::uint32_t*>(&value);
  holder.bind.is_unsigned = 1;
  return holder;
}

ParamHolder MakeBind(const std::uint64_t& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_LONGLONG;
  holder.bind.buffer = const_cast<std::uint64_t*>(&value);
  holder.bind.is_unsigned = 1;
  return holder;
}

// 浮点类型
ParamHolder MakeBind(const float& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_FLOAT;
  holder.bind.buffer = const_cast<float*>(&value);
  return holder;
}

ParamHolder MakeBind(const double& value)
{
  ParamHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_DOUBLE;
  holder.bind.buffer = const_cast<double*>(&value);
  return holder;
}

// ============================================================================
// 结果绑定 (用于 SELECT 查询的输出结果)
// ============================================================================

// 有符号整数类型
ResultHolder MakeResultBind(std::int8_t& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_TINY;
  holder.bind.buffer = &value;
  holder.bind.is_unsigned = 0;
  return holder;
}

ResultHolder MakeResultBind(std::int16_t& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_SHORT;
  holder.bind.buffer = &value;
  holder.bind.is_unsigned = 0;
  return holder;
}

ResultHolder MakeResultBind(std::int32_t& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_LONG;
  holder.bind.buffer = &value;
  holder.bind.is_unsigned = 0;
  return holder;
}

ResultHolder MakeResultBind(std::int64_t& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_LONGLONG;
  holder.bind.buffer = &value;
  holder.bind.is_unsigned = 0;
  return holder;
}

// 无符号整数类型
ResultHolder MakeResultBind(std::uint8_t& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_TINY;
  holder.bind.buffer = &value;
  holder.bind.is_unsigned = 1;
  return holder;
}

ResultHolder MakeResultBind(std::uint16_t& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_SHORT;
  holder.bind.buffer = &value;
  holder.bind.is_unsigned = 1;
  return holder;
}

ResultHolder MakeResultBind(std::uint32_t& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_LONG;
  holder.bind.buffer = &value;
  holder.bind.is_unsigned = 1;
  return holder;
}

ResultHolder MakeResultBind(std::uint64_t& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_LONGLONG;
  holder.bind.buffer = &value;
  holder.bind.is_unsigned = 1;
  return holder;
}

// 浮点类型
ResultHolder MakeResultBind(float& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_FLOAT;
  holder.bind.buffer = &value;
  return holder;
}

ResultHolder MakeResultBind(double& value)
{
  ResultHolder holder;
  holder.bind.buffer_type = MYSQL_TYPE_DOUBLE;
  holder.bind.buffer = &value;
  return holder;
}

}  // namespace utils