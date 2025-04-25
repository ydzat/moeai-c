#!/bin/bash

# 确保脚本在错误时停止
set -e

# 检查 glab 是否已安装
if ! command -v glab &> /dev/null; then
    echo "错误: GitLab CLI (glab) 未安装。请先安装: https://gitlab.com/gitlab-org/cli"
    exit 1
fi

# 获取项目根目录
PROJECT_ROOT=$(git rev-parse --show-toplevel)
cd "$PROJECT_ROOT"

# 获取最新标签
LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "")

if [ -z "$LATEST_TAG" ]; then
    echo "没有找到标签，将从第一个提交开始生成 CHANGELOG"
    FROM_PARAM=""
else
    echo "从最近的标签 $LATEST_TAG 生成 CHANGELOG"
    FROM_PARAM="--from $LATEST_TAG"
fi

# 获取当前版本
CURRENT_VERSION=$(grep -o 'VERSION_STR "[^"]*"' include/core/version.h | cut -d'"' -f2)
if [ -z "$CURRENT_VERSION" ]; then
    CURRENT_VERSION=$(date +"%Y.%m.%d")
    echo "未找到版本号，使用日期作为版本: $CURRENT_VERSION"
else
    echo "当前版本: $CURRENT_VERSION"
fi

# 生成 CHANGELOG
echo "正在生成 CHANGELOG..."
TEMP_CHANGELOG=$(mktemp)

# 使用 GitLab CLI 生成 CHANGELOG
glab changelog generate \
    --config-file .gitlab/changelog_config.yml \
    $FROM_PARAM \
    --version "$CURRENT_VERSION" > "$TEMP_CHANGELOG"

# 如果已存在 CHANGELOG 文件，将新生成的内容添加到开头
if [ -s "CHANGELOG.md" ]; then
    echo "将新内容添加到现有的 CHANGELOG.md"
    cat "$TEMP_CHANGELOG" <(echo) "CHANGELOG.md" > "CHANGELOG.md.new"
    mv "CHANGELOG.md.new" "CHANGELOG.md"
else
    echo "创建新的 CHANGELOG.md 文件"
    cp "$TEMP_CHANGELOG" "CHANGELOG.md"
fi

# 清理临时文件
rm "$TEMP_CHANGELOG"

echo "CHANGELOG 已更新: CHANGELOG.md"