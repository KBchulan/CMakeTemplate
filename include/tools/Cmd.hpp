/******************************************************************************
 *
 * @file       Cmd.hpp
 * @brief      解析的命令行参数
 *
 * @author     KBchulan
 * @date       2026/01/06
 * @history
 ******************************************************************************/

#ifndef CMD_HPP
#define CMD_HPP

#include <global/Global.hpp>

namespace tools
{

struct CmdOptions
{
  unsigned short port = global::server::DEFAULT_SERVER_PORT;
  bool show_help = false;
};

}  // namespace tools

#endif  // CMD_HPP