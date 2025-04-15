#!/bin/bash
# Optimized Kernel and DTB Build Script with Consolidated Folder Paths and Dynamic Job Count
# 此脚本构建内核、编译设备树、合并镜像并将其部署到 TFTP 目录。
# 通过传递参数 "nclean" 可以跳过 distclean 步骤，通过传递参数 "top_dir=<路径>" 指定 TOP_DIR。

# Exit immediately if any command fails
set -e

# ---------------------------
# 固定参数配置
# ---------------------------
ARCH="arm"
CROSS_COMPILE="arm-linux-gnueabihf-"
# 使用 nproc 命令获取系统实际 CPU 核数
JOBS=$(nproc)
TFTP_DIR="/home/alientek/linux/tftp"
KERNEL_VERSION="linux-4.1.15"
KERNEL_DEFCONFIG="alpha_defconfig"
IMAGE_NAME="imx6ull-alientek-emmc"

# ---------------------------
# 顶层路径及相关目录设置（所有内核源码、配置、设备树路径均相对于 TOP_DIR）
# ---------------------------

# 定义内核源码下各个文件夹路径
CURRENT_DIR=$(pwd)
PARENT_DIR=$(dirname "$CURRENT_DIR")

echo "当前目录: $CURRENT_DIR"
echo "上一级目录: $PARENT_DIR"

KERNEL_CONFIG_DIR="${PARENT_DIR}/config"
KERNEL_DTS_SRC_DIR="${PARENT_DIR}/dts"

KERNEL_SDK_CONFIG_DIR="${CURRENT_DIR}/arch/arm/configs"
KERNEL_BOOT_DIR="${CURRENT_DIR}/arch/arm/boot"

# 设备树编译目录（内核源码内的路径）
DTB_DIR="${CURRENT_DIR}/arch/arm/boot/dts"
# 内部编译输出或临时使用的目录
RELEASE_DIR="${CURRENT_DIR}/release"



# ---------------------------
# 参数解析（顺序可变）
# ---------------------------
CLEAN=0

for arg in "$@"; do
    case "$arg" in
        nclean|clean)
            CLEAN="$arg"
            ;;
        *)
            echo "未知参数：$arg"
            exit 1
            ;;
    esac
done


# ---------------------------
# 辅助函数: 检查命令是否存在
# ---------------------------
check_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: Required command '$1' not found. Please install it." >&2
        exit 1
    fi
}

# 确保 mkimage 已安装（属于 U-Boot 工具集的一部分）
check_command mkimage

# 如果 RELEASE_DIR 不存在则创建它
[ ! -d "${RELEASE_DIR}" ] && mkdir -p "${RELEASE_DIR}"

# ---------------------------
# 编译内核函数
# ---------------------------
build_kernel() {
    echo "==== 开始内核编译 ===="
    if [ $CLEAN = "clean" ]; then
        echo "==== 执行 distclean 步骤 ===="
        make ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" distclean
    else
        echo "==== 已跳过 distclean 步骤 ===="
    fi

    # 更新 kernel 配置文件
    pwd
    ls "${KERNEL_CONFIG_DIR}/${KERNEL_DEFCONFIG}"
    cp "${KERNEL_CONFIG_DIR}/${KERNEL_DEFCONFIG}" "${KERNEL_SDK_CONFIG_DIR}/${KERNEL_DEFCONFIG}"
    make ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" "${KERNEL_DEFCONFIG}"

    # 使用 nproc 动态指定并行任务数
    make ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" all -j$(nproc)

    cp "${KERNEL_BOOT_DIR}/zImage" "${RELEASE_DIR}"
    cp "${KERNEL_BOOT_DIR}/zImage" "${TFTP_DIR}"
}

# ---------------------------
# 设备树编译函数
# ---------------------------
build_dtbs() {
    echo "==== 开始设备树编译 ===="
    # 将必要的 DTS 和 DTSI 文件复制到内核源码树内对应位置
    cp "${KERNEL_DTS_SRC_DIR}/${IMAGE_NAME}.dts" "${DTB_DIR}/${IMAGE_NAME}.dts"
    cp "${KERNEL_DTS_SRC_DIR}/imx6ull.dtsi" "${DTB_DIR}/imx6ull.dtsi"
    
    make ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" dtbs

    cp "${KERNEL_BOOT_DIR}/dts/${IMAGE_NAME}.dtb" "${RELEASE_DIR}"
}

# ---------------------------
# 合并 zImage 与 DTB 成 uImage
# ---------------------------
merge_images() {
    echo "==== 合并 zImage 和 DTB 生成 uImage ===="
    cd "${RELEASE_DIR}"
    # 将 DTB 附加到 zImage 中，以嵌入设备树
    cat "${IMAGE_NAME}.dtb" >> zImage
    # 使用 mkimage 生成 uImage，包含合并后的 zImage
    mkimage -n "${KERNEL_VERSION}" -A arm -O linux -T kernel -C none -a 0x80800000 -e 0x80800040 -d zImage uImage

    echo "==== 部署内核映像到 TFTP 目录 ===="
    cp -f uImage "${TFTP_DIR}"
    ls -al "${TFTP_DIR}"
    cd -
}

# ---------------------------
# 主执行流程
# ---------------------------
echo "==== 开始构建流程 ===="
build_kernel
build_dtbs
merge_images
echo "==== 内核构建成功！ ===="