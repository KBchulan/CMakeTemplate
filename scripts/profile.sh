#!/bin/bash

# profile.sh - 使用 perf 和火焰图进行性能分析的脚本

# 任何命令失败都直接退出，未定义的变量也会报错
set -euo pipefail

# ------- 目录配置 -------

readonly SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

readonly PROJECT_ROOT="${SCRIPT_DIR}/.."

readonly BUILD_DIR="${PROJECT_ROOT}/build"

readonly BIN_DIR="${BUILD_DIR}/bin"
readonly PERF_DIR="${BUILD_DIR}/perf"
readonly FLAMEGRAPH_DIR="${BUILD_DIR}/flamegraph"

readonly PERF_DATA_FILE="${PERF_DIR}/perf.data"
readonly PERF_SCRIPT_FILE="${PERF_DIR}/out.perf"
readonly FOLDED_STACKS_FILE="${PERF_DIR}/out.folded"
readonly FLAMEGRAPH_SVG_FILE="${PERF_DIR}/flamegraph.svg"

# ------- 函数 -------

# 打印错误信息并退出
die() {
  echo "错误: $1" >&2
  exit 1
}

# 检查必要工具是否存在
check_dependencies() {
  command -v perf >/dev/null 2>&1 || die "'perf' 未安装，请运行 'sudo pacman -S perf' 或类似的命令进行安装"
  command -v pgrep >/dev/null 2>&1 || die "'pgrep' 未安装，无法自动查找 PID"
  [ -d "${FLAMEGRAPH_DIR}" ] || die "FlameGraph 目录未找到，请确保已在 build 目录中运行: cmake .. -DENABLE_PROFILING=ON"
  [ -f "${FLAMEGRAPH_DIR}/flamegraph.pl" ] || die "flamegraph.pl 脚本未找到"
  [ -f "${FLAMEGRAPH_DIR}/stackcollapse-perf.pl" ] || die "stackcollapse-perf.pl 脚本未找到"
}

# 查找可执行文件
find_executable() {
  [ -d "${BIN_DIR}" ] || die "'${BIN_DIR}' 目录不存在，请先构建项目"

  local executables=()
  mapfile -t executables < <(find "${BIN_DIR}" -maxdepth 1 -type f -executable)

  local count=${#executables[@]}

  if [ "${count}" -eq 0 ]; then
    die "在 '$(basename "${BIN_DIR}")' 中没有找到任何可执行文件"
  elif [ "${count}" -eq 1 ]; then
    echo "${executables[0]}"
  else
    echo "在 '$(basename "${BIN_DIR}")' 目录中找到多个可执行文件，请选择一个:" >&2

    local basenames=()
    for exe in "${executables[@]}"; do
      basenames+=("$(basename "$exe")")
    done

    PS3="请输入数字选择: "
    select choice in "${basenames[@]}"; do
      if [ -n "$choice" ]; then
        echo "${executables[$REPLY - 1]}"
        return
      else
        echo "无效的选择，请重试" >&2
      fi
    done
  fi
}


# ------- 主逻辑 -------

main() {
  check_dependencies

  local executable_path
  executable_path=$(find_executable)

  local executable_name
  executable_name=$(basename "${executable_path}")

  local prog_args=("$@")

  local display_path=$(realpath --relative-to="${PROJECT_ROOT}" "${executable_path}")
  local display_perf_dir=$(realpath --relative-to="${PROJECT_ROOT}" "${PERF_DIR}")

  echo
  echo "================================================="
  echo "可执行文件:   ${display_path}"
  echo "程序参数:     ${prog_args[*]}"
  echo "输出目录:     ${display_perf_dir}"
  echo "================================================="
  echo

  # 询问分析类型
  read -p "这是一个服务器类型的项目吗(y/n): " is_server
  is_server=$(echo "${is_server}" | tr '[:upper:]' '[:lower:]')

  # 确保 perf 目录存在
  mkdir -p "${PERF_DIR}"
  rm -f "${PERF_DATA_FILE}" "${PERF_SCRIPT_FILE}" "${FOLDED_STACKS_FILE}" "${FLAMEGRAPH_SVG_FILE}"

  if [[ "${is_server}" == "y" || "${is_server}" == "yes" ]]; then
    echo
    echo "----------- 进入服务器分析模式 -----------"
    echo "请在另一个终端启动服务: ${executable_name}"
    read -p "服务启动后，请按 Enter 键开始分析: "

    # 自动查找 PID
    local pid
    pid=$(pgrep -x "${executable_name}")

    if [ -z "${pid}" ]; then
      die "找不到名为 '${executable_name}' 的正在运行的进程"
    fi
    if [ "$(echo "${pid}" | wc -w)" -gt 1 ]; then
      die "找到多个名为 '${executable_name}' 的进程，请确保只有一个实例在运行，PIDs: ${pid}"
    fi

    echo "成功找到进程, PID: ${pid}"

    read -p "请输入采样持续时间 (秒): " duration
    if ! [[ "${duration}" =~ ^[1-9][0-9]*$ ]]; then
      die "持续时间必须是一个正整数"
    fi

    echo "将对 PID ${pid} 进行采样 ${duration} 秒"
    # -F 99: 采样频率 99Hz, -p: 指定 PID, -g: 记录调用图
    sudo perf record -F 99 -p "${pid}" -g -o "${PERF_DATA_FILE}" -- sleep "${duration}"

  else
    echo
    echo "----------- 进入普通程序分析模式 -----------"
    echo "正在运行程序并采集数据"

    # --: 分隔 perf 参数和要执行的程序
    sudo perf record -F 99 -g -o "${PERF_DATA_FILE}" -- "${executable_path}" "${prog_args[@]}"
  fi

  echo "----------- 采样完成 -----------"




  echo
  echo "----------- 正在生成火焰图 -----------"
  # 1. 将 perf.data 转换为可读格式
  echo "步骤 1/3: 运行 perf script"
  sudo perf script -i "${PERF_DATA_FILE}" > "${PERF_SCRIPT_FILE}"

  # 2. 折叠调用栈
  echo "步骤 2/3: 折叠调用栈"
  "${FLAMEGRAPH_DIR}/stackcollapse-perf.pl" "${PERF_SCRIPT_FILE}" > "${FOLDED_STACKS_FILE}"

  # 3. 生成 SVG 火焰图
  echo "步骤 3/3: 生成 SVG 文件"
  "${FLAMEGRAPH_DIR}/flamegraph.pl" "${FOLDED_STACKS_FILE}" > "${FLAMEGRAPH_SVG_FILE}"
  echo "----------- 火焰图生成完毕 -----------"




  echo
  echo "分析结果已保存到: ${FLAMEGRAPH_SVG_FILE}"
}

main "$@"