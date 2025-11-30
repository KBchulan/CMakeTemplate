#!/usr/bin/env bash
#
# Git hooks 安装脚本
# 将 .githooks 目录下的 hooks 复制到 .git/hooks/
#

set -e

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}开始安装 Git hooks...${NC}"
echo ""

# 检查是否在 git 仓库中
if [ ! -d ".git" ]; then
    echo -e "${YELLOW}错误: 当前目录不是 Git 仓库根目录${NC}"
    exit 1
fi

# 检查 .githooks 目录是否存在
if [ ! -d ".githooks" ]; then
    echo -e "${YELLOW}错误: .githooks 目录不存在${NC}"
    exit 1
fi

# 复制 hooks 并设置执行权限
for hook in .githooks/*; do
    if [ -f "$hook" ]; then
        hook_name=$(basename "$hook")
        echo -e "  ${GREEN}✓${NC} 安装 $hook_name"
        cp "$hook" ".git/hooks/$hook_name"
        chmod +x ".git/hooks/$hook_name"
    fi
done

echo ""
echo -e "${GREEN}✓ Git hooks 安装完成!${NC}"
echo ""
echo -e "${BLUE}已安装的 hooks:${NC}"
ls -1 .githooks/ | sed 's/^/  - /'
echo ""
echo -e "${BLUE}提示:${NC}"
echo "  - commit-msg: 检查 commit message 格式"
echo "  - post-commit: 自动更新 CHANGELOG.md 并 amend 到当前 commit"
echo ""
echo -e "${YELLOW}Commit message 格式要求:${NC}"
echo "  <type>: <description>"
echo "  或"
echo "  <type>(scope): <description>"
echo ""
echo -e "${YELLOW}支持的类型:${NC}"
echo "  feat, fix, docs, style, refactor, test, chore, perf, ci, build, revert"
echo ""
