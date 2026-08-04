#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
timeout_secs="${HUBOS_QEMU_TIMEOUT_SECS:-10}"
kernel_image="${1:-$repo_root/qemu-workspace/build/sel4_32.elf}"
initrd_image="${2:-$repo_root/qemu-workspace/build/loader.img}"
transcript_file="${3:-${QEMU_TRANSCRIPT_FILE:-${TMPDIR:-/tmp}/hubos-qemu-transcript.log}}"

mkdir -p "$(dirname -- "$transcript_file")"
rm -f "$transcript_file"

set +e
env QEMU_TRANSCRIPT_FILE="$transcript_file" \
  timeout "${timeout_secs}s" \
  "$repo_root/qemu-workspace/run-qemu.sh" \
  "$kernel_image" \
  "$initrd_image"
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  exit "$status"
fi

"$repo_root/scripts/verify-hubos-console-transcript.sh" "$transcript_file"
echo "HubOS QEMU console smoke test passed"
