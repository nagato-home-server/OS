#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
mode="${1:-native}"
upstream_root="${2:-$repo_root/src/hubos-upstream}"
source_json="$repo_root/seL4test-manifest/source-boundaries.json"

if [ ! -f "$source_json" ]; then
  echo "Missing source boundary manifest: $source_json" >&2
  exit 1
fi

required_native_repos="$(
  python3 - "$source_json" <<'PY'
import json
import pathlib
import sys

data = json.loads(pathlib.Path(sys.argv[1]).read_text())
for project in data["native_projects"] + data.get("driver_projects", []):
    print(project["name"].rsplit("/", 1)[-1].removesuffix(".git"))
PY
)"
optional_vm_backend_repo="$(
  python3 - "$source_json" <<'PY'
import json
import pathlib
import sys

data = json.loads(pathlib.Path(sys.argv[1]).read_text())
backends = data.get("optional_backends", [])
for backend in backends:
    repo = backend.get("repository")
    if repo:
        print(repo.rsplit("/", 1)[-1])
PY
)"

check_repo() {
  repo_name="$1"
  repo_path="$upstream_root/$repo_name"

  if [ ! -d "$repo_path" ]; then
    echo "Missing upstream repository: $repo_path" >&2
    exit 1
  fi

  if [ ! -d "$repo_path/.git" ]; then
    echo "Not a git checkout: $repo_path" >&2
    exit 1
  fi

  for marker in LICENSE.md LICENSE LICENSE.txt COPYING COPYING.md COPYING.BSD COPYRIGHT LICENSES RELICENSE.md; do
    if [ -e "$repo_path/$marker" ]; then
      return 0
    fi
  done

  echo "Missing license marker in: $repo_path" >&2
  exit 1
}

for repo_name in $required_native_repos; do
  check_repo "$repo_name"
done

case "$mode" in
  native)
    if [ -n "$optional_vm_backend_repo" ]; then
      if [ -d "$upstream_root/$optional_vm_backend_repo" ]; then
        check_repo "$optional_vm_backend_repo"
        echo "Optional VM backend present: $optional_vm_backend_repo"
      else
        echo "Optional VM backend absent: $optional_vm_backend_repo"
      fi
    fi
    ;;
  vm)
    if [ -z "$optional_vm_backend_repo" ]; then
      echo "No optional VM backend is defined in $source_json" >&2
      exit 1
    fi
    check_repo "$optional_vm_backend_repo"
    ;;
  *)
    echo "Unknown verification mode: $mode" >&2
    exit 1
    ;;
esac

echo "Verified upstream asset set in $upstream_root ($mode)"
