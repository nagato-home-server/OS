#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../../../" && pwd)"
rootfs_overlay="${LINUX_ROOTFS_OVERLAY:-$repo_root/vm/linux/rootfs/overlay}"
out_dir="${LINUX_ROOTFS_OUT:-$repo_root/build/vm/linux}"
image_name="${LINUX_ROOTFS_IMAGE_NAME:-rootfs.ext4}"
image_size="${LINUX_ROOTFS_SIZE:-512M}"

if [ ! -d "$rootfs_overlay" ]; then
  echo "Missing rootfs overlay: $rootfs_overlay" >&2
  exit 1
fi

mkdir -p "$out_dir"

if ! command -v mke2fs >/dev/null 2>&1; then
  echo "mke2fs is required to build $image_name" >&2
  exit 1
fi

tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/hubos-rootfs.XXXXXX")"
cleanup() {
  rm -rf "$tmp_dir"
}
trap cleanup EXIT INT TERM

staging="$tmp_dir/staging"
mkdir -p "$staging"
cp -R "$rootfs_overlay"/. "$staging"/

truncate -s "$image_size" "$out_dir/$image_name"
mke2fs -t ext4 -F -d "$staging" "$out_dir/$image_name" >/dev/null

printf '%s\n' "$out_dir/$image_name"
