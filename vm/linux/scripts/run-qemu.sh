#!/bin/sh

set -eu

shell_quote() {
  printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
}

qemu_binary="${QEMU_BINARY:-}"
qemu_memory="${QEMU_MEMORY:-2G}"
qemu_serial="${QEMU_SERIAL:-mon:stdio}"
qemu_bios="${QEMU_BIOS:-}"
qemu_cpu="${QEMU_CPU:-}"
qemu_log_file="${QEMU_LOG_FILE:-}"
qemu_transcript_file="${QEMU_TRANSCRIPT_FILE:-}"
system_image="${LIBVMM_SYSTEM_IMAGE:-}"
kernel_image="${LINUX_KERNEL_IMAGE:-}"
initramfs_image="${LINUX_INITRAMFS_IMAGE:-}"
rootfs_image="${LINUX_ROOTFS_IMAGE:-}"
device_tree_blob="${LINUX_DTB_IMAGE:-}"
kernel_cmdline="${LINUX_KERNEL_CMDLINE:-}"

if [ -z "$system_image" ] && [ -n "${1:-}" ] && [ -f "${1:-}" ]; then
  system_image="$1"
  shift
fi

if [ "$#" -gt 0 ]; then
  echo "Unexpected extra arguments: $*" >&2
  exit 1
fi

if [ -n "$system_image" ]; then
  qemu_machine="${QEMU_MACHINE:-virt}"
  if [ -z "$qemu_binary" ]; then
    qemu_binary="qemu-system-riscv64"
  fi
  set -- \
    -machine "$qemu_machine" \
    -nographic \
    -serial "$qemu_serial" \
    -kernel "$system_image" \
    -m "$qemu_memory"
  if [ -n "$qemu_log_file" ]; then
    mkdir -p "$(dirname -- "$qemu_log_file")"
    set -- -D "$qemu_log_file" -d guest_errors "$@"
  fi
  if [ -n "$qemu_transcript_file" ] && command -v script >/dev/null 2>&1; then
    mkdir -p "$(dirname -- "$qemu_transcript_file")"
    qemu_cmd="$(shell_quote "$qemu_binary")"
    for arg in "$@"; do
      qemu_cmd="$qemu_cmd $(shell_quote "$arg")"
    done
    exec script -q -f "$qemu_transcript_file" -c "$qemu_cmd"
  fi
  exec "$qemu_binary" "$@"
fi

if [ -z "$kernel_image" ] || [ -z "$initramfs_image" ]; then
  cat >&2 <<EOF
Usage:
  LIBVMM_SYSTEM_IMAGE=/path/to/system-image.elf $0
or
  LINUX_KERNEL_IMAGE=/path/to/bzImage \\
  LINUX_INITRAMFS_IMAGE=/path/to/initramfs.cpio.gz \\
  $0

Optional:
  LINUX_ROOTFS_IMAGE=/path/to/rootfs.ext4
  LINUX_DTB_IMAGE=/path/to/guest.dtb
EOF
  exit 1
fi

if [ -z "$qemu_binary" ]; then
  qemu_binary="qemu-system-x86_64"
fi

qemu_machine="${QEMU_MACHINE:-q35}"

if [ -z "$kernel_cmdline" ]; then
  if [ -n "$rootfs_image" ]; then
    kernel_cmdline="console=ttyS0,115200 earlyprintk=serial,0x3f8,115200 earlycon=serial,0x3f8,115200 loglevel=8 root=/dev/vda rw"
  else
    kernel_cmdline="console=ttyS0,115200 earlyprintk=serial,0x3f8,115200 earlycon=serial,0x3f8,115200 loglevel=8 root=/dev/ram0 rw rdinit=/init"
  fi
fi

set -- \
  -machine "$qemu_machine" \
  -nographic \
  -serial "$qemu_serial" \
  -kernel "$kernel_image" \
  -initrd "$initramfs_image" \
  -m "$qemu_memory" \
  -append "$kernel_cmdline"

if [ -n "$qemu_cpu" ]; then
  set -- "$@" -cpu "$qemu_cpu"
fi

if [ -n "$qemu_bios" ]; then
  set -- -bios "$qemu_bios" "$@"
fi

if [ -n "$device_tree_blob" ]; then
  case "$qemu_binary" in
    qemu-system-aarch64|qemu-system-arm*)
      set -- "$@" -dtb "$device_tree_blob"
      ;;
  esac
fi

if [ -n "$rootfs_image" ]; then
  set -- "$@" -drive "file=$rootfs_image,format=raw,if=virtio"
fi

if [ -n "$qemu_log_file" ]; then
  mkdir -p "$(dirname -- "$qemu_log_file")"
  set -- -D "$qemu_log_file" -d guest_errors "$@"
fi

if [ -n "$qemu_transcript_file" ] && command -v script >/dev/null 2>&1; then
  mkdir -p "$(dirname -- "$qemu_transcript_file")"
  qemu_cmd="$(shell_quote "$qemu_binary")"
  for arg in "$@"; do
    qemu_cmd="$qemu_cmd $(shell_quote "$arg")"
  done
  exec script -q -f "$qemu_transcript_file" -c "$qemu_cmd"
fi

exec "$qemu_binary" "$@"
