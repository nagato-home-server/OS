#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
lock_path="${1:-$repo_root/src/image-metadata/source-bundle-lock.json}"

python3 - "$repo_root" "$lock_path" <<'PY'
import json
import pathlib
import subprocess
import sys

repo_root = pathlib.Path(sys.argv[1])
lock_path = pathlib.Path(sys.argv[2])

if not lock_path.is_file():
    raise SystemExit(f"Missing source bundle lock file: {lock_path}")

data = json.loads(lock_path.read_text())
if data.get("schema_version") != 1:
    raise SystemExit(f"Unsupported schema_version in {lock_path}")

sources = data.get("sources")
if not isinstance(sources, dict) or not sources:
    raise SystemExit(f"No sources declared in {lock_path}")

for source_name, entry in sources.items():
    if not isinstance(entry, dict):
        raise SystemExit(f"Malformed source entry for {source_name}")
    rel_path = entry.get("path")
    revision = entry.get("revision")
    if not isinstance(rel_path, str) or not rel_path:
        raise SystemExit(f"Missing path for source entry: {source_name}")
    if not isinstance(revision, str) or len(revision) != 40:
        raise SystemExit(f"Missing or invalid revision for source entry: {source_name}")

    source_path = repo_root / rel_path
    if not source_path.is_dir():
        raise SystemExit(f"Missing source bundle directory for {source_name}: {source_path}")
    if not (source_path / ".git").exists():
        raise SystemExit(f"Source bundle is not a git checkout for {source_name}: {source_path}")

    actual_revision = subprocess.check_output(
        ["git", "-C", str(source_path), "rev-parse", "HEAD"],
        text=True,
    ).strip()
    if actual_revision != revision:
        raise SystemExit(
            f"Source bundle revision mismatch for {source_name}: "
            f"expected {revision}, got {actual_revision}"
        )

print(f"Verified source bundle lock: {lock_path}")
PY
