#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
workspace_dir="${1:-$repo_root/qemu-workspace}"
upstream_root="${HUBOS_UPSTREAM_ROOT:-$repo_root/src/hubos-upstream}"
target_board="${HUBOS_TARGET_BOARD:-qemu-x86_64_generic}"
qemu_platform="${HUBOS_QEMU_PLATFORM:-$("$repo_root/scripts/render-system-description.sh" --print-field microkit_board "$target_board")}"
qemu_memory="${HUBOS_QEMU_MEMORY:-2G}"
run_script="$workspace_dir/run-qemu.sh"

link_path() {
  source_path="$1"
  target_path="$2"

  if [ -L "$target_path" ] || [ -e "$target_path" ]; then
    if [ "$(readlink -f -- "$target_path")" = "$(readlink -f -- "$source_path")" ]; then
      return 0
    fi

    echo "Refusing to replace existing path: $target_path" >&2
    exit 1
  fi

  mkdir -p "$(dirname -- "$target_path")"
  ln -s "$source_path" "$target_path"
}

if [ ! -d "$upstream_root" ]; then
  echo "Missing upstream mirror root: $upstream_root" >&2
  exit 1
fi

"$repo_root/scripts/bootstrap-sel4-workspace.sh" "$workspace_dir"
"$repo_root/scripts/render-boot-package.sh" "$workspace_dir"

mkdir -p "$workspace_dir/tools" "$workspace_dir/projects"

link_path "$workspace_dir/upstream-mirror/seL4" "$workspace_dir/kernel"
link_path "$workspace_dir/upstream-mirror/seL4_tools" "$workspace_dir/tools/seL4"
link_path "$workspace_dir/upstream-mirror/seL4_libs" "$workspace_dir/projects/seL4_libs"
link_path "$workspace_dir/upstream-mirror/util_libs" "$workspace_dir/projects/util_libs"
link_path "$workspace_dir/upstream-mirror/sel4runtime" "$workspace_dir/projects/sel4runtime"
link_path "$workspace_dir/upstream-mirror/sel4test" "$workspace_dir/projects/sel4test"
link_path "$workspace_dir/upstream-mirror/sel4_projects_libs" "$workspace_dir/projects/sel4_projects_libs"
link_path "$workspace_dir/upstream-mirror/musllibc" "$workspace_dir/projects/musllibc"
link_path "$workspace_dir/upstream-mirror/sel4bench" "$workspace_dir/projects/sel4bench"
link_path "$workspace_dir/upstream-mirror/lwip" "$workspace_dir/projects/lwip"
link_path "$workspace_dir/upstream-mirror/microkit" "$workspace_dir/tools/microkit"
link_path "$workspace_dir/upstream-mirror/camkes-tool" "$workspace_dir/tools/camkes-tool"
link_path "$workspace_dir/upstream-mirror/nanopb" "$workspace_dir/tools/nanopb"
link_path "$workspace_dir/upstream-mirror/opensbi" "$workspace_dir/tools/opensbi"

link_path "$workspace_dir/upstream-mirror/seL4_tools/cmake-tool/default-CMakeLists.txt" \
  "$workspace_dir/CMakeLists.txt"

cat > "$workspace_dir/settings.cmake" <<EOF
#
# Copyright 2026 HubOS
#
# SPDX-License-Identifier: BSD-2-Clause
#

# QEMU bootstrap settings for the HubOS seL4 workspace.
# This selects the x86-64 generic platform so the seL4 side matches the
# intended x64 system layout and the Linux VM path can share the same host.

set(PLATFORM "${qemu_platform}" CACHE STRING "" FORCE)
set(SIMULATION ON CACHE BOOL "" FORCE)
set(RELEASE OFF CACHE BOOL "" FORCE)
set(VERIFICATION ON CACHE BOOL "" FORCE)
set(MCS ON CACHE BOOL "" FORCE)
set(DOMAINS OFF CACHE BOOL "" FORCE)
set(SMP OFF CACHE BOOL "" FORCE)
set(Sel4testAllowSettingsOverride OFF CACHE BOOL "" FORCE)
set(QEMU_MEMORY "${qemu_memory}" CACHE STRING "" FORCE)
EOF

cat > "$run_script" <<EOF
#!/bin/sh

set -eu

shell_quote() {
  printf "'%s'" "$(printf '%s' "\$1" | sed "s/'/'\\\\''/g")"
}

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
qemu_log_file="\${QEMU_LOG_FILE:-}"
qemu_transcript_file="\${QEMU_TRANSCRIPT_FILE:-}"

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
  qemu_cmd="\$(shell_quote qemu-system-x86_64)"
  for arg in "\$@"; do
    qemu_cmd="\$qemu_cmd \$(shell_quote "\$arg")"
  done
  exec script -q -f "\$qemu_transcript_file" -c "\$qemu_cmd"
fi

exec qemu-system-x86_64 "\$@"
EOF
chmod +x "$run_script"

cat > "$workspace_dir/README.qemu.md" <<'EOF'
# QEMU Bootstrap Workspace

This workspace is generated by `scripts/bootstrap-qemu-workspace.sh`.

Platform:

- `x86_64_generic`

Boot flow:

```sh
./run-qemu.sh <system-image.elf>
```

The `system-image.elf` argument is a seL4 system image, not a standalone Linux
kernel image. QEMU lands in the seL4 console first; Root Task then publishes
the service endpoints and starts the Linux VM from the seL4 control plane.
The current launcher still uses the `sel4_32.elf` compatibility copy, but the
canonical kernel artifact is `sel4.elf`.

Once the Linux VM is up, use the guest console checks below to validate the
runtime path:

- `ip a`
- `ip route`
- `lsblk`
- `fdisk -l`
- `dmesg | tail`

For capture, set one or both of:

- `QEMU_TRANSCRIPT_FILE` to record the interactive console with `script(1)`
- `QEMU_LOG_FILE` to write QEMU's debug log via `-D`
EOF

"$repo_root/scripts/render-system-description.sh" "$target_board" "$workspace_dir/hubos.system"

printf '%s\n' "Created QEMU bootstrap workspace at $workspace_dir"
printf '%s\n' "Generated QEMU launcher at $run_script"
printf '%s\n' "Generated boot package scaffold under $workspace_dir/boot-package"
printf '%s\n' "Pinned workspace root CMakeLists.txt to seL4_tools/default-CMakeLists.txt"
