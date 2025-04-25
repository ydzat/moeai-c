#!/bin/bash
#
# 多语言文档同步脚本
# 用途：在更新文档时自动同步各语言版本的文档内容
# 使用方法：./scripts/sync_translations.sh [文件路径]
#

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # 无颜色

# 目录定义
SOURCE_LANG="zh-cn"  # 源语言目录
TARGET_LANGS=("en" "de")  # 目标语言目录
DOC_ROOT="doc"
LANG_DIR="$DOC_ROOT/lang"
DESIGN_DIR="$DOC_ROOT/design"

# 初始化状态变量
success_count=0
failure_count=0
skipped_count=0

# 确保脚本在项目根目录下运行
if [ ! -d "$LANG_DIR" ]; then
    echo -e "${RED}Error: Please run this script from project root directory${NC}"
    exit 1
fi

# 显示帮助信息
show_help() {
    echo -e "${BLUE}MoeAI-C Document Translation Sync Tool${NC}"
    echo "Purpose: Automatically sync document content across language versions when updating"
    echo ""
    echo "Usage:"
    echo "  ./scripts/sync_translations.sh [options]"
    echo ""
    echo "Options:"
    echo "  --all                Sync all documents"
    echo "  --design             Sync design documents"
    echo "  --readme             Sync README documents"
    echo "  --file <file_path>   Sync specific document file"
    echo "  --help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  ./scripts/sync_translations.sh --design"
    echo "  ./scripts/sync_translations.sh --file doc/design/overview.md"
    echo ""
}

# 创建目标语言目录（如果不存在）
ensure_target_dirs() {
    local source_file=$1
    local source_dir=$(dirname "$source_file")
    local file_name=$(basename "$source_file")
    
    # 对每个目标语言
    for lang in "${TARGET_LANGS[@]}"; do
        # 替换路径中的源语言为目标语言
        local target_dir=${source_dir/$SOURCE_LANG/$lang}
        
        # 如果是设计文档目录，则需要特殊处理
        if [[ "$source_dir" == "$DESIGN_DIR" ]]; then
            target_dir="$LANG_DIR/$lang/design"
        fi
        
        # 创建目录（如果不存在）
        mkdir -p "$target_dir"
        
        echo -e "${BLUE}Ensuring target directory exists: ${target_dir}${NC}"
    done
}

# 为设计文档添加翻译通知
add_translation_notice() {
    local target_file=$1
    local source_file=$2
    local lang=$3
    
    # 确定语言名称
    local lang_name=""
    if [ "$lang" == "en" ]; then
        lang_name="English"
    elif [ "$lang" == "de" ]; then
        lang_name="German"
    else
        lang_name="$lang"
    fi
    
    # 确定源文档名称
    local source_name=$(echo "$source_file" | sed "s|.*/||g")
    
    # 添加翻译通知
    echo -e "<!-- 此文件是从 ${source_name} 自动翻译的 ${lang_name} 版本 -->\n<!-- This file is an automatically translated ${lang_name} version of ${source_name} -->\n" > "$target_file"
}

# 标记文件需要人工翻译
mark_for_translation() {
    local source_file=$1
    local target_file=$2
    local lang=$3
    
    # 添加翻译通知
    add_translation_notice "$target_file" "$source_file" "$lang"
    
    # 复制源文件内容
    cat "$source_file" >> "$target_file"
    
    # 添加翻译提醒
    echo -e "\n\n<!-- TODO: 此文档需要翻译成 ${lang} 语言 -->" >> "$target_file"
    
    echo -e "${YELLOW}Created ${target_file} and marked for translation${NC}"
}

# 同步单个文档
sync_file() {
    local source_file=$1
    local source_dir=$(dirname "$source_file")
    local file_name=$(basename "$source_file")
    
    # 检查源文件是否存在
    if [ ! -f "$source_file" ]; then
        echo -e "${RED}Error: Source file not found: ${source_file}${NC}"
        ((failure_count++))
        return 1
    fi
    
    echo -e "${BLUE}Processing: ${source_file}${NC}"
    
    # 确保目标目录存在
    ensure_target_dirs "$source_file"
    
    # 对每个目标语言
    for lang in "${TARGET_LANGS[@]}"; do
        # 确定目标文件路径
        local target_dir=""
        
        # 如果是设计文档目录，则需要特殊处理
        if [[ "$source_dir" == "$DESIGN_DIR" ]]; then
            target_dir="$LANG_DIR/$lang/design"
        else
            target_dir=${source_dir/$SOURCE_LANG/$lang}
        fi
        
        local target_file="$target_dir/$file_name"
        
        # 如果目标文件不存在，创建一个新文件并标记需要翻译
        if [ ! -f "$target_file" ]; then
            mark_for_translation "$source_file" "$target_file" "$lang"
            ((success_count++))
        else
            # 文件已存在，标记为已跳过
            echo -e "${YELLOW}Skipped existing file: ${target_file}${NC}"
            ((skipped_count++))
        fi
    done
}

# 同步设计文档
sync_design_docs() {
    echo -e "${GREEN}Syncing design documents...${NC}"
    
    # 获取所有设计文档
    local design_files=$(find "$DESIGN_DIR" -type f -name "*.md")
    
    # 对每个设计文档进行同步
    for file in $design_files; do
        sync_file "$file"
    done
}

# 同步README文档
sync_readme() {
    echo -e "${GREEN}Syncing README documents...${NC}"
    
    # 判断是否已有语言版本的README
    for lang in "${TARGET_LANGS[@]}"; do
        local target_file="$LANG_DIR/$lang/README.md"
        
        if [ ! -f "$target_file" ]; then
            echo -e "${RED}Error: ${lang} language README file not found: ${target_file}${NC}"
            ((failure_count++))
        else
            echo -e "${GREEN}${lang} language README file already exists${NC}"
            ((skipped_count++))
        fi
    done
}

# 同步所有文档
sync_all() {
    echo -e "${GREEN}Syncing all documents...${NC}"
    
    # 同步设计文档
    sync_design_docs
    
    # 同步README文档
    sync_readme
    
    # 将来可以添加更多类型的文档同步
}

# 主逻辑
main() {
    # 如果没有参数，显示帮助信息
    if [ $# -eq 0 ]; then
        show_help
        exit 0
    fi
    
    # 处理命令行参数
    while [[ $# -gt 0 ]]; do
        key="$1"
        case $key in
            --all)
                sync_all
                shift
                ;;
            --design)
                sync_design_docs
                shift
                ;;
            --readme)
                sync_readme
                shift
                ;;
            --file)
                sync_file "$2"
                shift
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                echo -e "${RED}Error: Unknown option: $1${NC}"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 显示统计信息
    echo ""
    echo -e "${GREEN}Sync completed!${NC}"
    echo -e "Success: ${success_count}"
    echo -e "Skipped: ${skipped_count}"
    echo -e "Failed: ${failure_count}"
    
    # 如果有失败的文件，返回非零退出码
    if [ $failure_count -gt 0 ]; then
        exit 1
    fi
}

# 执行主逻辑
main "$@"
