#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
workspace_dir="${1:-$repo_root/sel4-workspace}"
source_boundaries="$repo_root/seL4test-manifest/source-boundaries.json"
manifest_source="$repo_root/seL4test-manifest/hubos.xml"
manifest_target="$workspace_dir/hubos.xml"
upstream_root="${HUBOS_UPSTREAM_ROOT:-$repo_root/src/hubos-upstream}"

if [ ! -f "$source_boundaries" ]; then
  echo "Missing source boundary manifest: $source_boundaries" >&2
  exit 1
fi

mkdir -p "$workspace_dir"
"$repo_root/scripts/render-hubos-manifest.sh" "$source_boundaries" "$manifest_source"
cp "$manifest_source" "$manifest_target"
"$repo_root/scripts/stage-local-upstreams.sh" "$source_boundaries" "$upstream_root" "$workspace_dir"
"$repo_root/scripts/render-microkit-generated.sh" "$workspace_dir"

mkdir -p "$workspace_dir"
cat > "$workspace_dir/README.md" <<'EOF'
# seL4 Workspace Scaffold

This directory is a local workspace stub for the HubOS seL4 integration.

The pinned manifest is `hubos.xml`, generated from
`seL4test-manifest/source-boundaries.json` and copied from
`seL4test-manifest/hubos.xml`.

Typical next steps:

1. initialize a repo workspace using the pinned manifest
2. sync the official seL4 dependencies
3. build the kernel and user-space projects with the upstream seL4 toolchain
4. map the root task, resource registry, capability manager, session manager,
   hub, driver service, and network server into separate user-space services
5. consult the generated source boundary summary before selecting optional
   backend assets
6. use the staged `upstream-mirror/` tree when you want to inspect the local
   clones that back the manifest

The manifest is intentionally pinned to public GitHub repositories from the
seL4 ecosystem plus any board-specific driver repositories listed in
`driver_projects`.
EOF

mkdir -p "$workspace_dir"
cat > "$workspace_dir/next-steps.txt" <<'EOF'
repo init -u <workspace-root>/sel4-workspace -m hubos.xml
repo sync
EOF

printf '%s\n' "Created seL4 workspace scaffold at $workspace_dir"
printf '%s\n' "Pinned manifest copied to $manifest_target"
printf '%s\n' "Source boundaries rendered from $source_boundaries"
printf '%s\n' "Prepared Microkit generated workspace under $workspace_dir/generated"
