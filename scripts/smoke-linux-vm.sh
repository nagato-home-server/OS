#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
timeout_secs="${LINUX_VM_TIMEOUT_SECS:-15}"
transcript_file="${1:-${QEMU_TRANSCRIPT_FILE:-${TMPDIR:-/tmp}/linux-vm-transcript.log}}"

mkdir -p "$(dirname -- "$transcript_file")"
rm -f "$transcript_file"

set +e
env QEMU_TRANSCRIPT_FILE="$transcript_file" \
  timeout "${timeout_secs}s" \
  "$repo_root/scripts/run-linux-vm.sh"
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  exit "$status"
fi

"$repo_root/scripts/verify-linux-vm-transcript.sh" "$transcript_file"
echo "Standalone Linux VM smoke test passed"
