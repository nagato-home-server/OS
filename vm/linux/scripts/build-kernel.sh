#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../../../" && pwd)"
kernel_source="${LINUX_KERNEL_SOURCE:-$repo_root/src/hubos-upstream/linux}"
out_dir="${LINUX_KERNEL_OUT:-$repo_root/build/vm/linux/kernel}"
arch="${LINUX_KERNEL_ARCH:-x86_64}"
cross_compile="${LINUX_KERNEL_CROSS_COMPILE:-}"
kernel_defconfig="${LINUX_KERNEL_DEFCONFIG:-x86_64_defconfig}"
kernel_target="${LINUX_KERNEL_TARGET:-bzImage}"

if [ ! -d "$kernel_source" ]; then
  echo "Missing Linux kernel source tree: $kernel_source" >&2
  exit 1
fi

mkdir -p "$out_dir"

make_args="ARCH=$arch"
if [ -n "$cross_compile" ]; then
  make_args="$make_args CROSS_COMPILE=$cross_compile"
fi

if [ ! -f "$out_dir/.config" ]; then
  make -C "$kernel_source" O="$out_dir" $make_args "$kernel_defconfig"
fi

make -C "$kernel_source" O="$out_dir" $make_args "$kernel_target" modules

printf '%s\n' "$out_dir/$kernel_target"
