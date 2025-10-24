# !/usr/bin/env bash
# 本脚本主要用于执行 CI/CD 工作流,包含以下步骤:
#
# 1. 代码格式化
# 2. 项目构建, 包含静态代码检查
# 3. 代码覆盖率测试, 包含单元测试
# 4. 覆盖率阈值检查

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
COVERAGE_FILE="coverage.info.cleaned"
COVERAGE_THRESHOLD="75.0"

log() { printf '\n%s\n' "$*"; }

ensure_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "缺少必要工具：$1" >&2
    exit 1
  fi
}

check_all_tools() {
  log "==========> 检查必要工具 <=========="
  ensure_tool cmake
  ensure_tool ninja
  ensure_tool lcov
  ensure_tool genhtml
  ensure_tool bc
  log "所有工具均存在"
}

configure_project() {
  log "==========> 配置项目中 <=========="
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DENABLE_CPPCHECK=ON \
    -DBUILD_TESTING=ON \
    -DENABLE_COVERAGE=ON \
    -DBUILD_BENCHMARK=ON \
    -DENABLE_PROFILING=ON
}

run_format() {
  log "==========> 1. 进行代码格式化 <=========="
  bash "${ROOT_DIR}/scripts/format.sh"
}

run_build_and_static_analysis() {
  log "==========> 2. 进行项目构建与静态分析检查 <=========="
  cmake --build "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" --target cppcheck
}

run_coverage_test() {
  log "==========> 3. 运行单元测试与代码覆盖率测试 <=========="
  cmake --build "${BUILD_DIR}" --target coverage
}

check_coverage_threshold() {
  log "==========> 4. 检查代码覆盖率阈值, 要求达到 ${COVERAGE_THRESHOLD}% <=========="

  local report="${BUILD_DIR}/${COVERAGE_FILE}"
  if [[ ! -f "${report}" ]]; then
    echo "找不到覆盖率报告: ${report}" >&2
    exit 1
  fi

  local coverage
  coverage="$(lcov --summary "${report}" 2>&1 | awk '/lines/ {for(i=1;i<=NF;i++){if($i ~ /%$/){gsub("%","",$i); print $i; exit}}}')"

  if [[ -z "${coverage}" ]]; then
    echo "无法从 lcov 输出解析覆盖率" >&2
    exit 1
  fi

  printf '当前行覆盖率：%s%%\n' "${coverage}"
  if (( $(echo "${coverage} < ${COVERAGE_THRESHOLD}" | bc -l) )); then
    echo "覆盖率 ${coverage}% 低于阈值 ${COVERAGE_THRESHOLD}%" >&2
    exit 1
  fi
}

main() {
  log "==========> 工作流开始 <=========="

  check_all_tools               # 检查所有必要工具
  configure_project             # 配置构建系统
  run_format                    # 1. 格式化
  run_build_and_static_analysis # 2. 构建与静态分析
  run_coverage_test             # 3. 覆盖率测试 (含单测)
  check_coverage_threshold      # 4. 覆盖率阈值检查

  log "==========> 工作流结束, 全部通过 <=========="
}

main "$@"
