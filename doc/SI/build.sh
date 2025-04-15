#!/bin/bash

if [ -z "$1" ]; then
    echo "请输入工程根目录例如./trunk_v1.0/"
else
    echo "工程根目录为 $1"
fi
project_root_path=$1

rm -f kernel_uboot_hik.txt
kernel_path="./kernel_build/kernel.txt"
uboot_path="./uboot_build/uboot.txt"

# 通过脚本构建kernel uboot工程
kernel_sdk_path=$(find "$project_root_path" -maxdepth 1 -type d -name "kernel_sdk" 2>/dev/null)
uboot_sdk_path=$(find "$project_root_path" -maxdepth 1 -type d -name "uboot_sdk" 2>/dev/null)
modules_sdk_path=$(find "$project_root_path" -maxdepth 1 -type d -name "modules" 2>/dev/null)

./PF_Prj_Gen.sh "$kernel_sdk_path" ./kernel_build 2> /dev/null
mv ./kernel_build/FileList_SourceInsight.txt ./kernel_build/kernel.txt
echo "build kernel.txt ok"

./PF_Prj_Gen.sh "$uboot_sdk_path" ./uboot_build 2> /dev/null
mv ./uboot_build/FileList_SourceInsight.txt ./uboot_build/uboot.txt
echo "build uboot.txt ok"

function show_progress_bar() {
    local total=$1
    local current=$2
    local progress=$((current * 100 / total))
    local bar_length=50
    local num_chars=$((progress * bar_length / 100))

    # 确保 num_chars 不超过 bar_length
    if ((num_chars > bar_length)); then
        num_chars=$bar_length
    fi

    local filled_bar=$(printf "#%.0s" $(seq 1 "$num_chars"))
    local empty_bar=$(printf " %.0s" $(seq 1 "$((bar_length - num_chars))"))

    printf "\r[%s%s] %d%%" "$filled_bar" "$empty_bar" "$progress"
}


function build_kernel_uboot() {
    local path=$1
    file_line=$(wc -l < "$path")
    index=0
    ten_percent=$((file_line / 10))
    echo "$path文件行数：$file_line"
    if [ "$path" = "$kernel_path" ]; then
        while read -r line; do
            echo "$kernel_sdk_path/${line}" | sed 's/\r$//' >> kernel.txt
            ((index++))
            show_progress_bar "$file_line" "$index"
        done < "$path"
    elif [ "$path" = "$uboot_path" ]; then
        while read -r line; do
            echo "$uboot_sdk_path/${line}" | sed 's/\r$//' >> uboot.txt
            ((index++))
            show_progress_bar "$file_line" "$index"
        done < "$path"
    else
        echo "无效的路径参数：$path"
    fi
}

function build_hik_project() {
    dir_paths=("$modules_sdk_path")
    > hik_project.txt
    for dir_path in "${dir_paths[@]}"; do
        find "$dir_path" -type f \( -name "*.c" -o -name "*.h" \) -printf "%p\n" >> hik_project.txt
    done
}

build_kernel_uboot "$kernel_path"
build_kernel_uboot "$uboot_path"

cat uboot.txt >> kernel.txt
mv ./kernel.txt ./kernel_uboot.txt
rm ./uboot.txt
echo "kernel uboot 合并 OK"

build_hik_project
cat hik_project.txt >> kernel_uboot.txt
mv ./kernel_uboot.txt ./kernel_uboot_hik.txt
rm ./hik_project.txt
rm -rf ./kernel_build/
rm -rf ./uboot_build/
echo "SUCCESS !!!"
