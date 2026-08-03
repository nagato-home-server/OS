#!/bin/sh

set -eu

shell_quote() {
  printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
}

resolve_kernel_image() {
  candidate=""
  for candidate in \
    "$script_dir/kernel.elf" \
    "$script_dir/build/sel4.elf" \
    "$script_dir/build/sel4_32.elf" \
    "$script_dir/../src/hubos-upstream/microkit/build/x86_64_generic/debug/sel4/install/bin/kernel.elf" \
    "$script_dir/../src/hubos-upstream/microkit/release/microkit-sdk-2.2.0-dev/board/x86_64_generic/debug/elf/sel4.elf" \
    "$script_dir/../src/hubos-upstream/microkit/release/microkit-sdk-2.2.0-dev/board/x86_64_generic/debug/elf/sel4_32.elf"
  do
    if [ -e "$candidate" ] && [ -s "$candidate" ]; then
      readlink -f -- "$candidate"
      return 0
    fi
  done

  return 1
}

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 KERNEL_IMAGE INITRD_IMAGE [qemu-args...]" >&2
  exit 1
fi

kernel_image="$1"
initrd_image="$2"
shift 2
qemu_log_file="${QEMU_LOG_FILE:-}"
qemu_transcript_file="${QEMU_TRANSCRIPT_FILE:-}"

set -- \
  -cpu max,+fsgsbase \
  -nographic \
  -serial stdio \
  -monitor none \
  -kernel "$kernel_image" \
  -initrd "$initrd_image" \
  -m size=2G \
  "$@"

if [ -n "$qemu_log_file" ]; then
  mkdir -p "$(dirname -- "$qemu_log_file")"
  set -- -D "$qemu_log_file" -d guest_errors "$@"
fi

if [ -n "$qemu_transcript_file" ] && command -v script >/dev/null 2>&1; then
  mkdir -p "$(dirname -- "$qemu_transcript_file")"
  qemu_cmd="$(shell_quote qemu-system-x86_64)"
  for arg in "$@"; do
    qemu_cmd="$qemu_cmd $(shell_quote "$arg")"
  done
  exec script -q -f "$qemu_transcript_file" -c "$qemu_cmd"
fi

exec qemu-system-x86_64 "$@"
