#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
config_path="${HUBOS_RUNTIME_CONFIG:-$repo_root/config/hubos-runtime-config.json}"
bundle_index_path="${HUBOS_RUNTIME_BUNDLE_INDEX:-$repo_root/src/image-metadata/runtime-bundle-index.json}"
target_profile="${1:-}"

python3 - "$repo_root" "$config_path" "$bundle_index_path" "$target_profile" <<'PY'
import hashlib
import json
import pathlib
import sys

repo_root = pathlib.Path(sys.argv[1])
config_path = pathlib.Path(sys.argv[2])
bundle_index_path = pathlib.Path(sys.argv[3])
target_profile = sys.argv[4]

config = json.loads(config_path.read_text())
bundle_index = json.loads(bundle_index_path.read_text())

if bundle_index.get("schema_version") != 1:
    raise SystemExit(f"Unsupported schema_version in {bundle_index_path}")

bundle_entries = bundle_index.get("bundles")
if not isinstance(bundle_entries, list):
    raise SystemExit(f"Malformed runtime bundle index: {bundle_index_path}")

bundle_map = {}
for entry in bundle_entries:
    profile = entry.get("profile")
    version = entry.get("version")
    path = entry.get("path")
    if not isinstance(profile, str) or not isinstance(version, str) or not isinstance(path, str):
        raise SystemExit(f"Malformed bundle entry in {bundle_index_path}: {entry}")
    bundle_map[(profile, version)] = pathlib.Path(path)

path_backed_artifact_names = {
    "kernel",
    "initramfs",
    "rootfs",
    "device_tree_blob",
    "app",
}

updated = []

for profile in config.get("runtime_catalog", []):
    profile_id = profile["id"]
    if target_profile and profile_id != target_profile:
        continue

    version = profile.get("version")
    if not isinstance(version, str) or not version:
        raise SystemExit(f"Runtime profile is missing version: {profile_id}")

    bundle_rel_path = bundle_map.get((profile_id, version))
    if bundle_rel_path is None:
        raise SystemExit(f"No runtime bundle index entry for profile/version: {profile_id} {version}")

    bundle_dir = repo_root / bundle_rel_path
    manifest_path = bundle_dir / "manifest.json"
    if not manifest_path.is_file():
        raise SystemExit(f"Missing runtime bundle manifest for {profile_id}: {manifest_path}")

    manifest = json.loads(manifest_path.read_text())
    manifest["profile"] = profile_id
    manifest["version"] = version
    manifest["guest_class"] = profile.get("guest_class")
    manifest["os_family"] = profile.get("os_family")
    manifest["update_policy"] = profile.get("update_policy")

    artifact_names = []
    artifact_hashes = {}
    for artifact_name, artifact_path in profile.get("artifacts", {}).items():
        if artifact_name not in path_backed_artifact_names or not artifact_path:
            continue

        artifact_abs_path = repo_root / pathlib.Path(artifact_path)
        if not artifact_abs_path.is_file():
            raise SystemExit(
                f"Runtime artifact path does not exist for {profile_id}.{artifact_name}: {artifact_abs_path}"
            )
        try:
            relative_name = artifact_abs_path.relative_to(bundle_dir).as_posix()
        except ValueError as exc:
            raise SystemExit(
                f"Runtime artifact path is outside bundle directory for {profile_id}.{artifact_name}: "
                f"{artifact_abs_path}"
            ) from exc

        artifact_names.append(relative_name)
        artifact_hashes[relative_name] = hashlib.sha256(artifact_abs_path.read_bytes()).hexdigest()

    manifest["artifacts"] = artifact_names
    manifest["artifact_hashes"] = artifact_hashes
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    updated.append(str(manifest_path))

if target_profile and not updated:
    raise SystemExit(f"Unknown runtime profile: {target_profile}")

for path in updated:
    print(f"Refreshed runtime bundle manifest: {path}")
PY
