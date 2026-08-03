#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../../../" && pwd)"
initramfs_root="${LINUX_INITRAMFS_ROOT:-$repo_root/vm/linux/initramfs}"
out_dir="${LINUX_INITRAMFS_OUT:-$repo_root/build/vm/linux}"
image_name="${LINUX_INITRAMFS_IMAGE_NAME:-initramfs.cpio.gz}"

if [ ! -d "$initramfs_root" ]; then
  echo "Missing initramfs root: $initramfs_root" >&2
  exit 1
fi

mkdir -p "$out_dir"

tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/hubos-initramfs.XXXXXX")"
cleanup() {
  rm -rf "$tmp_dir"
}
trap cleanup EXIT INT TERM

cp -R "$initramfs_root"/. "$tmp_dir"/

if [ ! -x "$tmp_dir/init" ] && [ -f "$tmp_dir/init" ]; then
  chmod +x "$tmp_dir/init"
fi

(
  cd "$tmp_dir"
  find . -print | LC_ALL=C sort | cpio --quiet -o -H newc
) | gzip -9 > "$out_dir/$image_name"

printf '%s\n' "$out_dir/$image_name"
