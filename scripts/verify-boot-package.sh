#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
workspace_dir="${1:-$repo_root/qemu-workspace}"
case "$(basename -- "$workspace_dir")" in
  boot-package)
    package_dir="$workspace_dir"
    ;;
  *)
    package_dir="$workspace_dir/boot-package"
    ;;
esac
resolved_image="${HUBOS_BOOT_IMAGE:-}"
build_dir="${HUBOS_BOOT_IMAGE_BUILD_DIR:-}"

require_file() {
  path="$1"

  if [ ! -e "$path" ]; then
    echo "Missing boot package file: $path" >&2
    exit 1
  fi
}

require_file "$package_dir/boot-package.json"
require_file "$package_dir/boot-layout.txt"
require_file "$package_dir/README.md"
require_file "$package_dir/run-qemu.sh"

if ! grep -Fq 'x86_64_generic' "$package_dir/boot-package.json"; then
  echo "Boot package does not target x86_64_generic" >&2
  exit 1
fi

if ! grep -Fq 'KERNEL_IMAGE INITRD_IMAGE' "$package_dir/run-qemu.sh"; then
  echo "Boot package launcher does not use explicit kernel/initrd arguments" >&2
  exit 1
fi

if ! grep -Fq 'qemu-system-x86_64' "$package_dir/run-qemu.sh"; then
  echo "Boot package launcher does not target qemu-system-x86_64" >&2
  exit 1
fi

if ! grep -Fq 'QEMU_TRANSCRIPT_FILE' "$package_dir/run-qemu.sh"; then
  echo "Boot package launcher does not support transcript capture" >&2
  exit 1
fi

if ! grep -Fq 'QEMU_LOG_FILE' "$package_dir/run-qemu.sh"; then
  echo "Boot package launcher does not support debug log capture" >&2
  exit 1
fi

if [ -n "$build_dir" ] && [ -z "$resolved_image" ]; then
  resolved_image="$(sh "$repo_root/scripts/resolve-boot-image.sh" "$build_dir")"
fi

if [ -n "$resolved_image" ]; then
  if [ ! -L "$package_dir/system-image.elf" ]; then
    echo "Boot package is missing system-image.elf symlink" >&2
    exit 1
  fi

  if [ "$(readlink -f -- "$package_dir/system-image.elf")" != "$(readlink -f -- "$resolved_image")" ]; then
    echo "Boot package system-image.elf does not match resolved boot image" >&2
    exit 1
  fi
fi

echo "Verified HubOS boot package at $package_dir"
