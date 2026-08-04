#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
workspace_dir="${1:-$repo_root/qemu-workspace}"
build_dir="${HUBOS_QEMU_BUILD_DIR:-$workspace_dir/build}"
sdk_source="${HUBOS_MICROKIT_SOURCE:-$repo_root/src/hubos-upstream/microkit}"
sel4_source="${HUBOS_SEL4_SOURCE:-$repo_root/src/hubos-upstream/seL4}"
sdk_release="$sdk_source/release/microkit-sdk-2.2.0-dev"
target_board="${HUBOS_TARGET_BOARD:-qemu-x86_64_generic}"
board="${HUBOS_QEMU_BOARD:-$("$repo_root/scripts/render-system-description.sh" --print-field microkit_board "$target_board")}"
config="${HUBOS_QEMU_CONFIG:-debug}"
tool="${HUBOS_MICROKIT_TOOL:-$sdk_release/bin/microkit}"
build_sdk_script="$sdk_source/build_sdk.py"
board_dir="$sdk_release/board/$board/$config"
board_include="$board_dir/include"
board_lib="$board_dir/lib"
board_elf="$board_dir/elf"
system_file="$workspace_dir/hubos.system"
component_src_dir="$workspace_dir/generated"
kernel_image="$board_elf/sel4_32.elf"
kernel_image_64="$board_elf/sel4.elf"
kernel_build_image="$sdk_source/build/$board/$config/sel4/install/bin/kernel.elf"
initrd_image="$build_dir/loader.img"
gcc_prefix="${HUBOS_X86_64_GCC_PREFIX:-}"
root_task_runtime_sources="
  $repo_root/src/audit.c
  $repo_root/src/boot.c
  $repo_root/src/bus_manager.c
  $repo_root/src/capability_manager.c
  $repo_root/src/dma_manager.c
  $repo_root/src/device_server.c
  $repo_root/src/qemu_root_task_runtime.c
  $repo_root/src/driver_registry.c
  $repo_root/src/driver_service.c
  $repo_root/src/hub.c
  $repo_root/src/ipc.c
  $repo_root/src/memory_manager.c
  $repo_root/src/microkit_boot.c
  $repo_root/src/microkit_endpoint.c
  $repo_root/src/microkit_graph.c
  $repo_root/src/microkit_ipc.c
  $repo_root/src/microkit_kernel_glue.c
  $repo_root/src/microkit_runtime.c
  $repo_root/src/model.c
  $repo_root/src/network_server.c
  $repo_root/src/resource_registry.c
  $repo_root/src/root_task.c
  $repo_root/src/runtime_config.c
  $repo_root/src/session_manager.c
  $repo_root/src/service_endpoints.c
  $repo_root/src/sha256.c
  $repo_root/src/system.c
"

if [ -z "$gcc_prefix" ]; then
  if command -v x86_64-elf-gcc >/dev/null 2>&1; then
    gcc_prefix="x86_64-elf"
  elif command -v x86_64-linux-gnu-gcc >/dev/null 2>&1; then
    gcc_prefix="x86_64-linux-gnu"
  else
    echo "Missing x86-64 GCC toolchain: need x86_64-elf-gcc or x86_64-linux-gnu-gcc" >&2
    exit 1
  fi
fi

gcc_compiler="$(command -v "${gcc_prefix}-gcc" 2>/dev/null || command -v "${gcc_prefix}gcc")"
if command -v "${gcc_prefix}-nm" >/dev/null 2>&1; then
  gcc_nm="$(command -v "${gcc_prefix}-nm")"
else
  gcc_nm="$(command -v nm)"
fi

toolchain_wrapper_dir="${TMPDIR:-/tmp}/hubos-x86-toolchain"
mkdir -p "$toolchain_wrapper_dir"

make_tool_wrapper() {
  tool_name="$1"
  real_tool="$2"
  wrapper_path="$toolchain_wrapper_dir/${gcc_prefix}-${tool_name}"
  cat > "$wrapper_path" <<EOF
#!/bin/sh
exec "$real_tool" "\$@"
EOF
  chmod +x "$wrapper_path"
}

for tool_name in cpp objcopy nm ar ranlib ld as; do
  if ! command -v "${gcc_prefix}-${tool_name}" >/dev/null 2>&1; then
    if command -v "$tool_name" >/dev/null 2>&1; then
      make_tool_wrapper "$tool_name" "$(command -v "$tool_name")"
    fi
  fi
done

PATH="$toolchain_wrapper_dir:$PATH"

require_file() {
  path="$1"
  if [ ! -e "$path" ]; then
    echo "Missing required file: $path" >&2
    exit 1
  fi
}

ensure_sdk_runtime() {
  if [ -f "$board_include/kernel/gen_config.h" ] &&
     [ -f "$board_lib/libmicrokit.a" ] &&
     [ -f "$board_lib/microkit.ld" ] &&
     [ -s "$board_elf/sel4.elf" ] &&
     [ -s "$board_elf/sel4_32.elf" ] &&
     [ -f "$board_elf/monitor.elf" ] &&
     [ -f "$board_elf/initialiser.elf" ]; then
    if [ -z "$(find \
        "$build_sdk_script" \
        "$sdk_source/libmicrokit/src/main.c" \
        "$sdk_source/libmicrokit/src/dbg.c" \
        "$sdk_source/initialiser-offline/initialiser.c" \
        -newer "$board_lib/libmicrokit.a" -print -quit)" ] &&
       command -v "$gcc_nm" >/dev/null 2>&1 &&
       "$gcc_nm" -a "$board_elf/initialiser.elf" | grep -q "sel4_capdl_initializer_embedded_frames_data_start" &&
       "$gcc_nm" -a "$board_elf/initialiser.elf" | grep -q "sel4_capdl_initializer_serialized_spec_data_start"; then
      return 0
    fi
  fi

  if [ ! -f "$build_sdk_script" ]; then
    echo "Missing Microkit build script: $build_sdk_script" >&2
    exit 1
  fi

  if [ ! -d "$sel4_source" ]; then
    echo "Missing seL4 source tree: $sel4_source" >&2
    exit 1
  fi

  sdk_build_root="$sdk_source/build/$board/$config"
  if [ -d "$sdk_build_root" ]; then
    rm -rf "$sdk_build_root"
  fi

  (
    cd "$sdk_source"
    CARGO_NET_OFFLINE=true \
    CC="$gcc_compiler" \
    ASM="$gcc_compiler" \
    python3 "$build_sdk_script" \
      --sel4 "$sel4_source" \
      --boards "$board" \
      --configs "$config" \
      --skip-tool \
      --skip-docs \
      --skip-tar \
      --gcc-toolchain-prefix-x86_64="${gcc_prefix}"
  )
}

build_component() {
  slug="$1"
  src="$component_src_dir/$slug/main.c"
  obj="$build_dir/$slug.o"
  elf="$build_dir/$slug.elf"
  link_objects=""

  require_file "$src"

  "$gcc_compiler" \
    -std=gnu11 \
    -g -O2 \
    -nostdlib \
    -ffreestanding \
    -DCONFIG_PRINTING \
    -fno-pie \
    -fno-pic \
    -fno-stack-protector \
    -Wall \
    -Wextra \
    -Werror \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -march=x86-64 \
    -mtune=generic \
    -I"$component_src_dir/$slug" \
    -I"$board_include" \
    -I"$sdk_source/libmicrokit/include" \
    -I"$repo_root/include" \
    -c "$src" \
    -o "$obj"

  for runtime_src in $root_task_runtime_sources; do
    runtime_obj="$build_dir/${slug}-$(basename "$runtime_src" .c).o"
    "$gcc_compiler" \
      -std=gnu11 \
      -g -O2 \
      -nostdlib \
      -ffreestanding \
      -DCONFIG_PRINTING \
      -fno-pie \
      -fno-pic \
      -fno-stack-protector \
      -Wall \
      -Wextra \
      -Werror \
      -Wno-unused-function \
      -Wno-unused-parameter \
      -march=x86-64 \
      -mtune=generic \
      -I"$component_src_dir/$slug" \
      -I"$board_include" \
      -I"$sdk_source/libmicrokit/include" \
      -I"$repo_root/include" \
      -c "$runtime_src" \
      -o "$runtime_obj"
    link_objects="$link_objects $runtime_obj"
  done

  "$gcc_compiler" \
    -nostdlib \
    -ffreestanding \
    -Wl,-no-pie \
    -Wl,-T,"$board_lib/microkit.ld" \
    -L"$board_lib" \
    "$obj" \
    $link_objects \
    -lmicrokit \
    -o "$elf"
}

require_file "$workspace_dir/generated/README.md"
"$repo_root/scripts/render-system-description.sh" "$target_board" "$system_file"
require_file "$system_file"

ensure_sdk_runtime

mkdir -p "$build_dir"

if [ ! -f "$kernel_image" ]; then
  echo "Missing x86-64 Microkit kernel image: $kernel_image" >&2
  exit 1
fi

if [ ! -s "$kernel_build_image" ]; then
  echo "Missing built x86-64 kernel image: $kernel_build_image" >&2
  exit 1
fi

ln -sfn "$(readlink -f -- "$kernel_build_image")" "$build_dir/sel4.elf"
objcopy_tool="$(command -v "${gcc_prefix}-objcopy" 2>/dev/null || command -v objcopy)"
"$objcopy_tool" -O elf32-i386 "$build_dir/sel4.elf" "$build_dir/sel4_32.elf"

if [ -f "$kernel_image_64" ]; then
  ln -sfn "$(readlink -f -- "$kernel_image_64")" "$build_dir/sel4.sdk.elf"
fi

for slug in \
  root-task \
  resource-registry \
  capability-manager \
  session-manager \
  hub \
  memory-manager \
  dma-manager \
  driver-registry \
  driver-loader \
  bus-managers \
  driver-service \
  network-server \
  device-server
do
  build_component "$slug"
done

"$tool" "$system_file" \
  --search-path "$build_dir" \
  --board "$board" \
  --config "$config" \
  -o "$initrd_image" \
  -r "$build_dir/report.txt"

HUBOS_BOOT_IMAGE_BUILD_DIR="$build_dir" \
  "$repo_root/scripts/render-boot-package.sh" \
  "$workspace_dir"

HUBOS_BOOT_IMAGE_BUILD_DIR="$build_dir" \
  "$repo_root/scripts/verify-boot-package.sh" \
  "$workspace_dir"

printf '%s\n' "Built HubOS QEMU system image at $build_dir/loader.img"
printf '%s\n' "Staged x86-64 seL4 kernel image at $build_dir/sel4.elf"
if [ -L "$build_dir/sel4.elf" ] || [ -f "$build_dir/sel4.elf" ]; then
  printf '%s\n' "Kept QEMU compatibility copy at $build_dir/sel4_32.elf"
fi
printf '%s\n' "Boot package rendered at $workspace_dir/boot-package"
printf '%s\n' "x86-64 kernel image expected at $kernel_image"
