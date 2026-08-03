#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
if [ "$#" -gt 0 ]; then
  workspace_dir="$1"
else
  workspace_dir="$(mktemp -d "${TMPDIR:-/tmp}/hubos-sel4-verify.XXXXXX")"
fi
source_boundaries="$repo_root/seL4test-manifest/source-boundaries.json"

cleanup() {
  if [ "${HUBOS_KEEP_SEL4_VERIFY:-0}" = "1" ]; then
    return
  fi

  rm -rf "$workspace_dir"
}

trap cleanup EXIT INT TERM

"$repo_root/scripts/bootstrap-sel4-workspace.sh" "$workspace_dir"

manifest="$workspace_dir/hubos.xml"
generated_dir="$workspace_dir/generated"

if [ ! -f "$manifest" ]; then
  echo "Missing seL4 workspace manifest: $manifest" >&2
  exit 1
fi

if [ ! -f "$source_boundaries" ]; then
  echo "Missing source boundary manifest: $source_boundaries" >&2
  exit 1
fi

if [ ! -f "$generated_dir/source-boundaries.json" ]; then
  echo "Missing generated source boundary manifest: $generated_dir/source-boundaries.json" >&2
  exit 1
fi

if ! cmp -s "$generated_dir/source-boundaries.json" "$source_boundaries"; then
  echo "Generated source boundaries do not match $source_boundaries" >&2
  exit 1
fi

for required in \
  "seL4/seL4.git" \
  "seL4/microkit.git" \
  "seL4/sel4runtime.git" \
  "lwip-tcpip/lwip.git"
do
  if ! grep -Fq "$required" "$manifest"; then
    echo "Pinned manifest does not include required upstream: $required" >&2
    exit 1
  fi
done

if [ ! -d "$generated_dir" ]; then
  echo "Missing generated workspace directory: $generated_dir" >&2
  exit 1
fi

component_count="$(find "$generated_dir" -mindepth 2 -maxdepth 2 -name component.json | wc -l | awk '{print $1}')"
if [ "$component_count" -lt 13 ]; then
  echo "Expected at least 13 generated Microkit components, found $component_count" >&2
  exit 1
fi

for entrypoint in \
  "root-task" \
  "resource-registry" \
  "capability-manager" \
  "session-manager" \
  "hub" \
  "driver-service" \
  "network-server" \
  "device-server"
do
  main_file="$generated_dir/$entrypoint/main.c"
  if [ ! -f "$main_file" ]; then
    echo "Missing generated entrypoint stub: $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "void init(void)" "$main_file"; then
    echo "Missing init() entrypoint in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "void notified(microkit_channel ch)" "$main_file"; then
    echo "Missing notified() entrypoint in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_generated_dispatch_notification" "$main_file"; then
    echo "Missing notification dispatch helper in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "HUBOS_MICROKIT_COMPONENT_KIND" "$main_file"; then
    echo "Missing generated component kind metadata in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_generated_matches_badge" "$main_file"; then
    echo "Missing badge validation helper in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_generated_matches_service" "$main_file"; then
    echo "Missing service validation helper in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)" "$main_file"; then
    echo "Missing protected() entrypoint in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_microkit_transport_frame_from_msginfo" "$main_file" &&
     ! grep -Fq "hubos_microkit_kernel_dispatch_protected" "$main_file"; then
    echo "Missing Microkit transport or kernel protected read path in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_microkit_transport_request_decode" "$main_file" &&
     ! grep -Fq "hubos_microkit_kernel_dispatch_protected" "$main_file"; then
    echo "Missing Microkit request decode or kernel protected dispatch path in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_microkit_transport_synthesize_response" "$main_file" &&
     ! grep -Fq "hubos_microkit_kernel_dispatch_protected" "$main_file"; then
    echo "Missing Microkit protected dispatch path in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_microkit_transport_response_encode" "$main_file" &&
     ! grep -Fq "hubos_microkit_kernel_dispatch_protected" "$main_file"; then
    echo "Missing Microkit response encode or kernel protected dispatch path in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_microkit_transport_frame_to_mrs" "$main_file" &&
     ! grep -Fq "hubos_microkit_kernel_dispatch_protected" "$main_file"; then
    echo "Missing Microkit transport frame write or kernel protected dispatch path in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "hubos_microkit_transport_frame_to_msginfo" "$main_file" &&
     ! grep -Fq "hubos_microkit_kernel_dispatch_protected" "$main_file"; then
    echo "Missing Microkit transport msginfo bridge or kernel protected dispatch path in $main_file" >&2
    exit 1
  fi

  if ! grep -Fq "seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)" "$main_file"; then
    echo "Missing fault() entrypoint in $main_file" >&2
    exit 1
  fi
done

if [ ! -f "$generated_dir/manifest.json" ]; then
  echo "Missing generated manifest: $generated_dir/manifest.json" >&2
  exit 1
fi

echo "seL4 integration scaffold verified at $workspace_dir"
