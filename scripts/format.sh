#!/bin/bash

# 脚本功能：使用 clang-format 格式化项目自有源代码（src, include, tests 目录）。
# 支持 --check 参数来检查格式而不修改文件。

set -e

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECT_ROOT="$SCRIPT_DIR/.."

SOURCE_DIRS=(
    "$PROJECT_ROOT/src"
    "$PROJECT_ROOT/include"
    "$PROJECT_ROOT/tests"
)

FILE_PATTERNS=(-name "*.cc" -o -name "*.cpp" -o -name "*.cxx" -o -name "*.h" -o -name "*.hpp" -o -name "*.hxx")

CHECK_MODE=false

for arg in "$@"; do
    case $arg in
        --check)
            CHECK_MODE=true
            ;;
        -h|--help)
            echo "使用方法: $0 [--check]"
            echo "  --check    检查格式但不修改文件"
            exit 0
            ;;
    esac
done

if [ ${#SOURCE_DIRS[@]} -eq 0 ]; then
    echo "错误：没有指定任何源代码目录进行格式化"
    exit 1
fi

if [ "$CHECK_MODE" = true ]; then
    echo "正在检查指定的源代码目录格式: src/, include/, tests/"
    find "${SOURCE_DIRS[@]}" -type f \( "${FILE_PATTERNS[@]}" \) -print0 | xargs -0 clang-format --dry-run -Werror
    echo "格式检查完成"
else
    echo "正在格式化指定的源代码目录: src/, include/, tests/"
    find "${SOURCE_DIRS[@]}" -type f \( "${FILE_PATTERNS[@]}" \) -print0 | xargs -0 clang-format -i
    echo "格式化完成"
fi