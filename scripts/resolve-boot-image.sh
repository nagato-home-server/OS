#!/bin/sh

set -eu

abspath() {
  path="$1"
  dir="$(dirname -- "$path")"
  base="$(basename -- "$path")"
  printf '%s/%s\n' "$(CDPATH= cd -- "$dir" && pwd -P)" "$base"
}

resolve_from_dir() {
  build_dir="$1"
  if [ ! -d "$build_dir" ]; then
    return 1
  fi

  for candidate in \
    "$build_dir/loader.img" \
    "$build_dir/images/loader.img" \
    "$build_dir/images/boot-package/loader.img" \
    "$build_dir/images/system-image.elf" \
    "$build_dir/images/fw_payload.elf" \
    "$build_dir/boot-package/system-image.elf"
  do
    if [ -f "$candidate" ]; then
      abspath "$candidate"
      return 0
    fi
  done

  candidate="$(find "$build_dir" -type f \( -name 'loader.img' -o -name '*.img' -o -name 'fw_payload.elf' -o -name '*.elf' \) | head -n 1)"
  if [ -n "$candidate" ]; then
    abspath "$candidate"
    return 0
  fi

  return 1
}

if [ -n "${HUBOS_BOOT_IMAGE:-}" ] && [ -f "${HUBOS_BOOT_IMAGE:-}" ]; then
  abspath "$HUBOS_BOOT_IMAGE"
  exit 0
fi

if [ -n "${HUBOS_BOOT_IMAGE_BUILD_DIR:-}" ]; then
  if resolved="$(resolve_from_dir "$HUBOS_BOOT_IMAGE_BUILD_DIR")"; then
    printf '%s\n' "$resolved"
    exit 0
  fi
fi

if [ "$#" -gt 0 ]; then
  if resolved="$(resolve_from_dir "$1")"; then
    printf '%s\n' "$resolved"
    exit 0
  fi
fi

echo "Could not resolve a boot image. Set HUBOS_BOOT_IMAGE or HUBOS_BOOT_IMAGE_BUILD_DIR." >&2
exit 1
