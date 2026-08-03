#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../../../" && pwd)"
out_dir="${LINUX_BUNDLE_OUT:-$repo_root/build/vm/linux}"
kernel_image="${LINUX_KERNEL_IMAGE:-$repo_root/build/vm/linux/kernel/bzImage}"
initramfs_image="${LINUX_INITRAMFS_IMAGE:-$repo_root/build/vm/linux/initramfs.cpio.gz}"
rootfs_image="${LINUX_ROOTFS_IMAGE:-$repo_root/build/vm/linux/rootfs.ext4}"
device_tree_blob="${LINUX_DTB_IMAGE:-}"
kernel_cmdline="${LINUX_KERNEL_CMDLINE:-}"

mkdir -p "$out_dir"

for path in "$kernel_image" "$initramfs_image"; do
  if [ ! -f "$path" ]; then
    echo "Missing Linux VM artifact: $path" >&2
    exit 1
  fi
done

if [ -n "$rootfs_image" ] && [ ! -f "$rootfs_image" ]; then
  rootfs_image=""
fi

if [ -z "$kernel_cmdline" ]; then
  if [ -n "$rootfs_image" ]; then
    kernel_cmdline="console=ttyS0,115200 earlyprintk=serial,0x3f8,115200 earlycon=serial,0x3f8,115200 loglevel=8 root=/dev/vda rw"
  else
    kernel_cmdline="console=ttyS0,115200 earlyprintk=serial,0x3f8,115200 earlycon=serial,0x3f8,115200 loglevel=8 root=/dev/ram0 rw rdinit=/init"
  fi
fi

{
  printf '%s\n' "kernel_image=$kernel_image"
  printf '%s\n' "initramfs_image=$initramfs_image"
  if [ -n "$rootfs_image" ]; then
    printf '%s\n' "rootfs_image=$rootfs_image"
  fi
  if [ -n "$device_tree_blob" ]; then
    printf '%s\n' "device_tree_blob=$device_tree_blob"
  fi
  printf '%s\n' "kernel_cmdline=$kernel_cmdline"
  printf '%s\n' "backend=libvmm"
} > "$out_dir/linux-vm.bundle"

printf '%s\n' "$out_dir/linux-vm.bundle"
