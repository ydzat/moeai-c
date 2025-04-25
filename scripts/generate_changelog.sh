#!/bin/bash
###
 # @Author: @ydzat
 # @Date: 2025-04-25 02:16:00
 # @LastEditors: @ydzat
 # @LastEditTime: 2025-04-25 23:28:06
 # @Description: 
### 

# 确保脚本在错误时停止
set -e

# 检查 glab 是否已安装
if ! command -v glab &> /dev/null; then
    echo "Error: GitLab CLI (glab) not installed. Please install first: https://gitlab.com/gitlab-org/cli"
    exit 1
fi

# 获取项目根目录
PROJECT_ROOT=$(git rev-parse --show-toplevel)
cd "$PROJECT_ROOT"

# 获取最新标签
LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "")

if [ -z "$LATEST_TAG" ]; then
    echo "No tags found, generating CHANGELOG from first commit"
    FROM_PARAM=""
else
    echo "Generating CHANGELOG from latest tag $LATEST_TAG"
    FROM_PARAM="--from $LATEST_TAG"
fi

# 获取当前版本
CURRENT_VERSION=$(grep -o 'VERSION_STR "[^"]*"' include/core/version.h | cut -d'"' -f2)
if [ -z "$CURRENT_VERSION" ]; then
    CURRENT_VERSION=$(date +"%Y.%m.%d")
    echo "No version found, using date as version: $CURRENT_VERSION"
else
    echo "Current version: $CURRENT_VERSION"
fi

# 生成 CHANGELOG
echo "Generating CHANGELOG..."
TEMP_CHANGELOG=$(mktemp)

# 使用 GitLab CLI 生成 CHANGELOG
glab changelog generate \
    --config-file .gitlab/changelog_config.yml \
    $FROM_PARAM \
    --version "$CURRENT_VERSION" > "$TEMP_CHANGELOG"

# 如果已存在 CHANGELOG 文件，将新生成的内容添加到开头
if [ -s "CHANGELOG.md" ]; then
    echo "Adding new content to existing CHANGELOG.md"
    cat "$TEMP_CHANGELOG" <(echo) "CHANGELOG.md" > "CHANGELOG.md.new"
    mv "CHANGELOG.md.new" "CHANGELOG.md"
else
    echo "Creating new CHANGELOG.md file"
    cp "$TEMP_CHANGELOG" "CHANGELOG.md"
fi

# 清理临时文件
rm "$TEMP_CHANGELOG"

echo "CHANGELOG updated: CHANGELOG.md"
