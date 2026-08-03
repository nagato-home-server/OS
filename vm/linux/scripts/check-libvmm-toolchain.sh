#!/bin/sh

set -eu

missing=0

require() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "missing: $1" >&2
    missing=1
  fi
}

require make
require dtc
require clang
require llvm-objcopy
require qemu-system-x86_64

if [ -z "${MICROKIT_SDK:-}" ]; then
  echo "missing: MICROKIT_SDK" >&2
  missing=1
elif [ ! -d "${MICROKIT_SDK}" ]; then
  echo "missing: MICROKIT_SDK points to a non-directory: ${MICROKIT_SDK}" >&2
  missing=1
fi

if [ "$missing" -ne 0 ]; then
  cat >&2 <<'EOF'
Install the libvmm dependencies first, then rerun the check.

Ubuntu/Debian baseline from upstream:
  sudo apt install -y make clang lld llvm qemu-system-arm qemu-system-x86 \
    device-tree-compiler iasl

For this repository's x86_64 example path, also ensure MICROKIT_SDK points to
the microkit 2.2.0-dev SDK used by libvmm/examples/simple.
EOF
  exit 1
fi

echo "libvmm toolchain looks ready"
