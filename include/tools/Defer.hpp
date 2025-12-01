/******************************************************************************
 *
 * @file       Defer.hpp
 * @brief      模仿 Golang 实现 defer 宏
 *
 * @author     KBchulan
 * @date       2025/12/01
 * @history
 *****************************************************************************/

#ifndef DEFER_HPP
#define DEFER_HPP

#include <utility>

template <typename F>
class [[nodiscard]] Defer
{
public:
  constexpr explicit Defer(F _func) : func(std::move(_func))
  {
  }

  constexpr ~Defer()
  {
    func();
  }

  Defer(const Defer&) = delete;
  Defer& operator=(const Defer&) = delete;

private:
  F func;
};

#define DEFER_CONCAT_(x, y) x##y
#define DEFER_CONCAT(x, y) DEFER_CONCAT_(x, y)
#define defer(code) Defer DEFER_CONCAT(_defer_, __LINE__)([&]() { code; })

#endif  // DEFER_HPP
