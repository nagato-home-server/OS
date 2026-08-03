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
system_image="${HUBOS_BOOT_IMAGE:-}"
build_dir="${HUBOS_BOOT_IMAGE_BUILD_DIR:-}"
kernel_image=""
qemu_platform="${HUBOS_QEMU_PLATFORM:-x86_64_generic}"
qemu_memory="${HUBOS_QEMU_MEMORY:-2G}"
qemu_binary="${HUBOS_QEMU_BINARY:-qemu-system-x86_64}"

mkdir -p "$package_dir"

if [ -n "$system_image" ] && [ ! -f "$system_image" ]; then
  echo "Missing boot image: $system_image" >&2
  exit 1
fi

if [ -z "$system_image" ] && [ -n "$build_dir" ]; then
  system_image="$(sh "$repo_root/scripts/resolve-boot-image.sh" "$build_dir")"
fi

if [ -n "$build_dir" ] && [ -f "$build_dir/sel4_32.elf" ]; then
  kernel_image="$build_dir/sel4_32.elf"
elif [ -n "$build_dir" ] && [ -f "$build_dir/sel4.elf" ]; then
  kernel_image="$build_dir/sel4.elf"
fi

if [ -n "$system_image" ]; then
  ln -sfn "$(readlink -f -- "$system_image")" "$package_dir/system-image.elf"
fi

if [ -n "$kernel_image" ]; then
  ln -sfn "$(readlink -f -- "$kernel_image")" "$package_dir/kernel.elf"
fi

cat > "$package_dir/boot-package.json" <<EOF
{
  "name": "hubos-boot-package",
  "platform": "${qemu_platform}",
  "qemu_binary": "${qemu_binary}",
  "qemu_machine": "pc",
  "memory": "${qemu_memory}",
  "boot_image_build_dir": $(if [ -n "$build_dir" ]; then printf '"%s"' "$build_dir"; else printf 'null'; fi),
  "system_image": $(if [ -n "$system_image" ]; then printf '"system-image.elf"'; else printf 'null'; fi),
  "kernel_image": $(if [ -n "$kernel_image" ]; then printf '"kernel.elf"'; else printf 'null'; fi),
  "artifacts": [
    "boot-package.json",
    "boot-layout.txt",
    "run-qemu.sh"
  ]
}
EOF

cat > "$package_dir/boot-layout.txt" <<'EOF'
HubOS boot package layout:

- boot-package.json
- boot-layout.txt
- run-qemu.sh
- system-image.elf (optional, loader.img provided by HUBOS_BOOT_IMAGE)
- kernel.elf (optional, x86-64 seL4 kernel ELF provided by the build dir)

The package is intentionally a staging layout. It becomes bootable once a
system image is attached and the upstream x86-64 seL4/Microkit build has
produced the kernel/user-space payload.
EOF

cat > "$package_dir/README.md" <<'EOF'
# HubOS Boot Package

This directory is a staging point for a QEMU bootable HubOS image.

It records:

- the QEMU invocation shape
- the selected seL4 Microkit platform
- the boot image attachment point

If `system-image.elf` is present, `run-qemu.sh` will boot the seL4 payload and
land in the seL4 console first. Root Task remains responsible for starting the
Linux VM from the control plane. On x86-64 the canonical kernel payload is the
Microkit-generated `sel4.elf` image, with `sel4_32.elf` kept as the QEMU
compatibility copy used by the current launcher.

Otherwise, pass the seL4 system image path as the first argument.

For log capture, set one or both of:

- `QEMU_TRANSCRIPT_FILE` to record the terminal session with `script(1)`
- `QEMU_LOG_FILE` to write QEMU's debug log via `-D`
EOF

cat > "$package_dir/run-qemu.sh" <<EOF
#!/bin/sh

set -eu

shell_quote() {
  printf "'%s'" "$(printf '%s' "\$1" | sed "s/'/'\\\\''/g")"
}

script_dir="\$(CDPATH= cd -- "\$(dirname -- "\$0")" && pwd)"
qemu_log_file="\${QEMU_LOG_FILE:-}"
qemu_transcript_file="\${QEMU_TRANSCRIPT_FILE:-}"

if [ "\$#" -ge 2 ]; then
  kernel_image="\$1"
  initrd_image="\$2"
  shift 2
elif candidate_kernel="\$(for candidate in \
  "\$script_dir/kernel.elf" \
  "\$script_dir/../build/sel4.elf" \
  "\$script_dir/../build/sel4_32.elf" \
  "\$script_dir/../../src/hubos-upstream/microkit/build/x86_64_generic/debug/sel4/install/bin/kernel.elf" \
  "\$script_dir/../../src/hubos-upstream/microkit/release/microkit-sdk-2.2.0-dev/board/x86_64_generic/debug/elf/sel4.elf" \
  "\$script_dir/../../src/hubos-upstream/microkit/release/microkit-sdk-2.2.0-dev/board/x86_64_generic/debug/elf/sel4_32.elf"
do
  if [ -e "\$candidate" ] && [ -s "\$candidate" ]; then
    readlink -f -- "\$candidate"
    break
  fi
done)" && [ -e "\$script_dir/system-image.elf" ]; then
  kernel_image="\$candidate_kernel"
  initrd_image="\$(readlink -f -- "\$script_dir/system-image.elf")"
else
  echo "Usage: \$0 KERNEL_IMAGE INITRD_IMAGE [qemu-args...]" >&2
  exit 1
fi

set -- \
  -machine pc \
  -cpu qemu64,+fsgsbase,+pdpe1gb,+xsave \
  -nographic \
  -serial stdio \
  -monitor none \
  -kernel "\$kernel_image" \
  -initrd "\$initrd_image" \
  -m size=${qemu_memory} \
  "\$@"

if [ -n "\$qemu_log_file" ]; then
  mkdir -p "\$(dirname -- "\$qemu_log_file")"
  set -- -D "\$qemu_log_file" -d guest_errors "\$@"
fi

if [ -n "\$qemu_transcript_file" ] && command -v script >/dev/null 2>&1; then
  mkdir -p "\$(dirname -- "\$qemu_transcript_file")"
  qemu_cmd="\$(shell_quote ${qemu_binary})"
  for arg in "\$@"; do
    qemu_cmd="\$qemu_cmd \$(shell_quote "\$arg")"
  done
  exec script -q -f "\$qemu_transcript_file" -c "\$qemu_cmd"
fi

exec ${qemu_binary} "\$@"
EOF
chmod +x "$package_dir/run-qemu.sh"

printf '%s\n' "Rendered HubOS boot package at $package_dir"
