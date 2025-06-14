#!/bin/bash
#
# 用法示例：
#   ./build.sh ./ ./output.txt
#
# 将会把 /path/to/target_directory 及其子目录下的所有文件路径（绝对路径）
# 输出到 /path/to/output.txt 中，每行一个文件完整路径。

# 检查参数个数
if [ $# -ne 2 ]; then
  echo "Usage: $0 <target_directory> <output_txt>"
  exit 1
fi

TARGET_DIR="$1"
OUTPUT_TXT="$2"

# 检查 TARGET_DIR 是否存在且为目录
if [ ! -d "$TARGET_DIR" ]; then
  echo "Error: '$TARGET_DIR' 不是一个有效目录。"
  exit 1
fi

# 如果输出文件已存在，则先清空
> "$OUTPUT_TXT"

# 使用 find 命令递归查找所有普通文件，并将绝对路径写入输出文件
find "$TARGET_DIR" -type f -print >> "$OUTPUT_TXT"

echo "已将 '$TARGET_DIR' 下的所有文件路径写入到 '$OUTPUT_TXT'。"
