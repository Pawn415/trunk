#!/usr/bin/env bash
# Build and publish the i.MX6ULL U-Boot image. This script never flashes eMMC.
set -Eeuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UBOOT_DIR="${UBOOT_DIR:-$SCRIPT_DIR/uboot_sdk}"
OUT_DIR="${OUT_DIR:-/home/he/build/uboot-alientek}"
NFS_DIR="${NFS_DIR:-/home/he/nfsroot}"
TFTP_DIR="${TFTP_DIR:-/srv/tftp}"
ARCH="${ARCH:-arm}"
CROSS_COMPILE="${CROSS_COMPILE:-/home/he/toolchains/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-}"
DEFCONFIG="${DEFCONFIG:-uboot_defconfig}"
JOBS="${JOBS:-$(nproc)}"

die() { echo "ERROR: $*" >&2; exit 1; }
[[ -d "$UBOOT_DIR" ]] || die "U-Boot source directory not found: $UBOOT_DIR"
[[ -f "$UBOOT_DIR/configs/$DEFCONFIG" ]] || die "defconfig not found: $UBOOT_DIR/configs/$DEFCONFIG"
command -v "${CROSS_COMPILE}gcc" >/dev/null || die "cross compiler not found: ${CROSS_COMPILE}gcc"
command -v sudo >/dev/null || die "sudo is required to publish to $TFTP_DIR"

mkdir -p "$OUT_DIR" "$NFS_DIR"

# U-Boot 2016.03 rejects O= builds if generated files remain in the source tree.
# mrproper removes only generated U-Boot files; it preserves source patches.
echo "==> Clean stale generated U-Boot source files"
make -C "$UBOOT_DIR" mrproper

echo "==> Configure: $DEFCONFIG"
make -C "$UBOOT_DIR" O="$OUT_DIR" "$DEFCONFIG"

echo "==> Build U-Boot with $JOBS job(s)"
make -C "$UBOOT_DIR" O="$OUT_DIR" ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" -j"$JOBS"

IMX="$OUT_DIR/u-boot.imx"
BIN="$OUT_DIR/u-boot.bin"
[[ -s "$IMX" ]] || die "build did not produce $IMX"
[[ -s "$BIN" ]] || die "build did not produce $BIN"

# Keep versioned artifacts in NFS for review and recovery.
install -m 0644 "$IMX" "$NFS_DIR/u-boot-imx6ull-alientek-emmc.imx"
install -m 0644 "$BIN" "$NFS_DIR/u-boot-imx6ull-alientek-emmc.bin"
(
    cd "$NFS_DIR"
    sha256sum u-boot-imx6ull-alientek-emmc.imx u-boot-imx6ull-alientek-emmc.bin \
        > SHA256SUMS-uboot-alientek
)

# cmd_updateb.c downloads exactly u-boot.imx, so publish that name to TFTP
# only after a successful build. This copy does not write to the board.
echo "==> Publish to TFTP: $TFTP_DIR"
sudo -v
sudo install -m 0644 "$IMX" "$TFTP_DIR/u-boot.imx"
sudo install -m 0644 "$IMX" "$TFTP_DIR/u-boot-imx6ull-alientek-emmc.imx"

echo
echo "Build succeeded."
echo "NFS : $NFS_DIR/u-boot-imx6ull-alientek-emmc.imx"
echo "TFTP: $TFTP_DIR/u-boot.imx"
echo "Board update command (only when you explicitly want to flash): updateb"
