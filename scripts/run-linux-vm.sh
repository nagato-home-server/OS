#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
bundle_dir="${LINUX_VM_BUNDLE_DIR:-$repo_root/build/vm/linux/libvmm-x86_64}"
kernel_image="${LINUX_KERNEL_IMAGE:-$bundle_dir/bzImage}"
initramfs_image="${LINUX_INITRAMFS_IMAGE:-$bundle_dir/rootfs.cpio.gz}"
rootfs_image="${LINUX_ROOTFS_IMAGE:-}"
device_tree_blob="${LINUX_DTB_IMAGE:-}"
kernel_cmdline="${LINUX_KERNEL_CMDLINE:-}"
qemu_binary="${QEMU_BINARY:-qemu-system-x86_64}"

if [ ! -f "$kernel_image" ]; then
  echo "Missing Linux kernel image: $kernel_image" >&2
  exit 1
fi

if [ ! -f "$initramfs_image" ]; then
  echo "Missing Linux initramfs image: $initramfs_image" >&2
  exit 1
fi

if [ -n "$rootfs_image" ] && [ ! -f "$rootfs_image" ]; then
  echo "Missing Linux rootfs image: $rootfs_image" >&2
  exit 1
fi

if [ -z "$kernel_cmdline" ]; then
  if [ -n "$rootfs_image" ]; then
    kernel_cmdline="console=ttyS0,115200 earlyprintk=serial,0x3f8,115200 earlycon=serial,0x3f8,115200 loglevel=8 root=/dev/vda rw"
  else
    kernel_cmdline="console=ttyS0,115200 earlyprintk=serial,0x3f8,115200 earlycon=serial,0x3f8,115200 loglevel=8 root=/dev/ram0 rw rdinit=/init"
  fi
fi

exec env \
  LINUX_KERNEL_IMAGE="$kernel_image" \
  LINUX_INITRAMFS_IMAGE="$initramfs_image" \
  LINUX_ROOTFS_IMAGE="$rootfs_image" \
  LINUX_DTB_IMAGE="$device_tree_blob" \
  LINUX_KERNEL_CMDLINE="$kernel_cmdline" \
  QEMU_BINARY="$qemu_binary" \
  "$repo_root/vm/linux/scripts/run-qemu.sh" \
  "$@"
