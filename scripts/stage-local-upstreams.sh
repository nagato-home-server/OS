#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
source_json="${1:-$repo_root/seL4test-manifest/source-boundaries.json}"
upstream_root="${2:-$repo_root/src/hubos-upstream}"
workspace_dir="${3:-$repo_root/sel4-workspace}"
staging_dir="$workspace_dir/upstream-mirror"

mkdir -p "$staging_dir"

python3 - "$source_json" "$upstream_root" "$staging_dir" <<'PY'
import json
import os
import pathlib
import sys

source_json = pathlib.Path(sys.argv[1])
upstream_root = pathlib.Path(sys.argv[2])
staging_dir = pathlib.Path(sys.argv[3])

data = json.loads(source_json.read_text())
native_projects = data["native_projects"]
optional_backends = data.get("optional_backends", [])
driver_projects = data.get("driver_projects", [])

def local_name(repo_name: str) -> str:
    return repo_name.rsplit("/", 1)[-1].removesuffix(".git")

def stage(repo_name: str) -> None:
    source = upstream_root / local_name(repo_name)
    target = staging_dir / local_name(repo_name)
    if not source.exists():
        raise SystemExit(f"Missing local upstream mirror: {source}")
    if target.exists() or target.is_symlink():
        if target.is_dir() and not target.is_symlink():
            raise SystemExit(f"Refusing to replace existing directory: {target}")
        target.unlink()
    os.symlink(source, target)

for project in native_projects:
    stage(project["name"])

for project in driver_projects:
    stage(project["name"])

for backend in optional_backends:
    repo_name = backend.get("repository")
    if not repo_name:
        continue
    source = upstream_root / local_name(repo_name)
    if source.exists():
        stage(repo_name)

readme = staging_dir / "README.md"
readme.write_text(
    "# Local Upstream Mirror\n\n"
    "This directory contains symlinks to the repositories cloned under "
    "`src/hubos-upstream/`.\n\n"
    "It is staged automatically from `seL4test-manifest/source-boundaries.json`.\n"
)
PY

printf '%s\n' "Staged local upstream mirror at $staging_dir"
