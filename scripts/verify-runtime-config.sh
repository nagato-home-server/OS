#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
config_path="${1:-${HUBOS_RUNTIME_CONFIG:-$repo_root/config/hubos-runtime-config.json}}"
bundle_index_path="${HUBOS_RUNTIME_BUNDLE_INDEX:-$repo_root/src/image-metadata/runtime-bundle-index.json}"

HUBOS_RUNTIME_CONFIG="$config_path" "$repo_root/scripts/hubos-runtime-setup.sh" verify

python3 - "$repo_root" "$config_path" "$bundle_index_path" <<'PY'
import json
import hashlib
import pathlib
import sys

repo_root = pathlib.Path(sys.argv[1])
config_path = pathlib.Path(sys.argv[2])
bundle_index_path = pathlib.Path(sys.argv[3])
source_bundle_lock_path = repo_root / "src/image-metadata/source-bundle-lock.json"

config = json.loads(config_path.read_text())
bundle_index = json.loads(bundle_index_path.read_text())
source_bundle_lock = json.loads(source_bundle_lock_path.read_text())

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

source_bundles = source_bundle_lock.get("sources")
if source_bundle_lock.get("schema_version") != 1 or not isinstance(source_bundles, dict):
    raise SystemExit(f"Malformed source bundle lock: {source_bundle_lock_path}")

catalog = config.get("runtime_catalog", [])
path_backed_artifact_names = {
    "kernel",
    "initramfs",
    "rootfs",
    "device_tree_blob",
    "app",
}
for profile in catalog:
    profile_id = profile["id"]
    version = profile.get("version")
    guest_class = profile.get("guest_class")
    os_family = profile.get("os_family")
    update_policy = profile.get("update_policy")
    artifacts = profile.get("artifacts", {})

    if not isinstance(version, str) or not version:
        raise SystemExit(f"Runtime profile is missing version: {profile_id}")
    if not isinstance(update_policy, str) or not update_policy:
        raise SystemExit(f"Runtime profile is missing update_policy: {profile_id}")

    bundle_rel_path = bundle_map.get((profile_id, version))
    if bundle_rel_path is None:
      raise SystemExit(
          f"No runtime bundle index entry for profile/version: {profile_id} {version}"
      )

    bundle_dir = repo_root / bundle_rel_path
    if not bundle_dir.is_dir():
        raise SystemExit(f"Missing runtime bundle directory for {profile_id}: {bundle_dir}")

    manifest_path = bundle_dir / "manifest.json"
    if not manifest_path.is_file():
        raise SystemExit(f"Missing runtime bundle manifest for {profile_id}: {manifest_path}")

    manifest = json.loads(manifest_path.read_text())
    if manifest.get("profile") != profile_id:
        raise SystemExit(f"Runtime manifest profile mismatch for {profile_id}: {manifest_path}")
    if manifest.get("version") != version:
        raise SystemExit(f"Runtime manifest version mismatch for {profile_id}: {manifest_path}")
    if manifest.get("guest_class") != guest_class:
        raise SystemExit(f"Runtime manifest guest_class mismatch for {profile_id}: {manifest_path}")
    if manifest.get("os_family") != os_family:
        raise SystemExit(f"Runtime manifest os_family mismatch for {profile_id}: {manifest_path}")
    if manifest.get("update_policy") != update_policy:
        raise SystemExit(f"Runtime manifest update_policy mismatch for {profile_id}: {manifest_path}")

    manifest_artifacts = manifest.get("artifacts")
    if not isinstance(manifest_artifacts, list):
        raise SystemExit(f"Runtime manifest artifacts must be a list: {manifest_path}")
    manifest_hashes = manifest.get("artifact_hashes")
    if not isinstance(manifest_hashes, dict):
        raise SystemExit(f"Runtime manifest artifact_hashes must be an object: {manifest_path}")
    manifest_source_bundles = manifest.get("source_bundles")
    if not isinstance(manifest_source_bundles, list):
        raise SystemExit(f"Runtime manifest source_bundles must be a list: {manifest_path}")
    for source_bundle_name in manifest_source_bundles:
        if source_bundle_name not in source_bundles:
            raise SystemExit(
                f"Runtime manifest references unknown source bundle {source_bundle_name}: {manifest_path}"
            )

    referenced_paths = []
    for artifact_name, artifact_path in artifacts.items():
        if artifact_name not in path_backed_artifact_names:
            continue
        if not artifact_path:
            continue
        artifact_rel_path = pathlib.Path(artifact_path)
        artifact_abs_path = repo_root / artifact_rel_path
        if not artifact_abs_path.is_file():
            raise SystemExit(
                f"Runtime artifact path does not exist for {profile_id}.{artifact_name}: {artifact_abs_path}"
            )
        try:
            relative_to_bundle = artifact_abs_path.relative_to(bundle_dir)
        except ValueError as exc:
            raise SystemExit(
                f"Runtime artifact path is outside bundle directory for {profile_id}.{artifact_name}: "
                f"{artifact_abs_path}"
            ) from exc
        relative_name = relative_to_bundle.as_posix()
        referenced_paths.append(relative_name)
        actual_hash = hashlib.sha256(artifact_abs_path.read_bytes()).hexdigest()
        expected_hash = manifest_hashes.get(relative_name)
        if expected_hash != actual_hash:
            raise SystemExit(
                f"Runtime artifact hash mismatch for {profile_id}.{artifact_name}: "
                f"expected {expected_hash}, got {actual_hash}"
            )

    missing_manifest_artifacts = sorted(set(referenced_paths) - set(manifest_artifacts))
    if missing_manifest_artifacts:
        raise SystemExit(
            f"Runtime manifest is missing artifact entries for {profile_id}: "
            f"{', '.join(missing_manifest_artifacts)}"
        )
    missing_hashes = sorted(set(referenced_paths) - set(manifest_hashes))
    if missing_hashes:
        raise SystemExit(
            f"Runtime manifest is missing artifact hashes for {profile_id}: "
            f"{', '.join(missing_hashes)}"
        )

print(f"Runtime bundle linkage verified: {bundle_index_path}")
PY
