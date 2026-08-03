#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
if [ "$#" -gt 0 ]; then
  workspace_dir="$1"
else
  workspace_dir="$(mktemp -d "${TMPDIR:-/tmp}/hubos-qemu-verify.XXXXXX")"
fi

if [ ! -e "$workspace_dir/CMakeLists.txt" ]; then
  "$repo_root/scripts/bootstrap-qemu-workspace.sh" "$workspace_dir"
fi

require_file() {
  path="$1"

  if [ ! -e "$path" ]; then
    echo "Missing QEMU scaffold file: $path" >&2
    exit 1
  fi
}

require_link() {
  path="$1"
  expected="$2"

  if [ ! -L "$path" ]; then
    echo "Missing QEMU scaffold symlink: $path" >&2
    exit 1
  fi

  if [ "$(readlink -f -- "$path")" != "$(readlink -f -- "$expected")" ]; then
    echo "Unexpected symlink target for $path" >&2
    exit 1
  fi
}

require_file "$workspace_dir/CMakeLists.txt"
require_file "$workspace_dir/settings.cmake"
require_file "$workspace_dir/run-qemu.sh"
require_file "$workspace_dir/README.qemu.md"
require_file "$workspace_dir/hubos.system"
require_file "$workspace_dir/boot-package/boot-package.json"
require_file "$workspace_dir/boot-package/boot-layout.txt"
require_file "$workspace_dir/boot-package/README.md"
require_file "$workspace_dir/boot-package/run-qemu.sh"
require_link "$workspace_dir/kernel" "$workspace_dir/upstream-mirror/seL4"
require_link "$workspace_dir/tools/seL4" "$workspace_dir/upstream-mirror/seL4_tools"
require_link "$workspace_dir/projects/sel4test" "$workspace_dir/upstream-mirror/sel4test"
require_link "$workspace_dir/projects/sel4runtime" "$workspace_dir/upstream-mirror/sel4runtime"
require_link "$workspace_dir/projects/seL4_libs" "$workspace_dir/upstream-mirror/seL4_libs"

if ! grep -Fq 'x86_64_generic' "$workspace_dir/settings.cmake"; then
  echo "QEMU scaffold settings do not select x86_64_generic" >&2
  exit 1
fi

if ! grep -Fq 'KERNEL_IMAGE INITRD_IMAGE' "$workspace_dir/run-qemu.sh"; then
  echo "QEMU launcher does not use explicit kernel/initrd arguments" >&2
  exit 1
fi

if ! grep -Fq 'qemu-system-x86_64' "$workspace_dir/run-qemu.sh"; then
  echo "QEMU launcher does not target qemu-system-x86_64" >&2
  exit 1
fi

if ! grep -Fq 'QEMU_TRANSCRIPT_FILE' "$workspace_dir/run-qemu.sh"; then
  echo "QEMU launcher does not support transcript capture" >&2
  exit 1
fi

if ! grep -Fq 'QEMU_LOG_FILE' "$workspace_dir/run-qemu.sh"; then
  echo "QEMU launcher does not support debug log capture" >&2
  exit 1
fi

if ! grep -Fq 'seL4 console' "$workspace_dir/README.qemu.md"; then
  echo "QEMU workspace README does not describe the seL4 console first" >&2
  exit 1
fi

if ! grep -Fq '<protection_domain name="Root Task"' "$workspace_dir/hubos.system"; then
  echo "QEMU system description does not include Root Task" >&2
  exit 1
fi

if ! grep -Fq '<channel>' "$workspace_dir/hubos.system"; then
  echo "QEMU system description does not include any Microkit channels" >&2
  exit 1
fi

if ! grep -Fq 'pp="true"' "$workspace_dir/hubos.system"; then
  echo "QEMU system description does not include any protected-call channels" >&2
  exit 1
fi

if ! grep -Fq 'pd="Device Server"' "$workspace_dir/hubos.system"; then
  echo "QEMU system description does not wire the Device Server into the system graph" >&2
  exit 1
fi

if ! grep -Fq 'KERNEL_IMAGE INITRD_IMAGE' "$workspace_dir/boot-package/run-qemu.sh"; then
  echo "Boot package launcher does not use explicit kernel/initrd arguments" >&2
  exit 1
fi

if ! grep -Fq 'qemu-system-x86_64' "$workspace_dir/boot-package/run-qemu.sh"; then
  echo "Boot package launcher does not target qemu-system-x86_64" >&2
  exit 1
fi

if ! grep -Fq 'QEMU_TRANSCRIPT_FILE' "$workspace_dir/boot-package/run-qemu.sh"; then
  echo "Boot package launcher does not support transcript capture" >&2
  exit 1
fi

if ! grep -Fq 'QEMU_LOG_FILE' "$workspace_dir/boot-package/run-qemu.sh"; then
  echo "Boot package launcher does not support debug log capture" >&2
  exit 1
fi

if ! grep -Fq 'Root Task remains responsible for starting' "$workspace_dir/boot-package/README.md"; then
  echo "Boot package README does not describe Linux VM startup from Root Task" >&2
  exit 1
fi

if ! grep -Fq 'Linux VM from the control plane' "$workspace_dir/boot-package/README.md"; then
  echo "Boot package README does not mention the Linux VM control plane" >&2
  exit 1
fi

echo "Verified QEMU scaffold at $workspace_dir"
