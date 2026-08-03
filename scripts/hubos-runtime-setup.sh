#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
config_path="${HUBOS_RUNTIME_CONFIG:-$repo_root/config/hubos-runtime-config.json}"
editor="${VISUAL:-${EDITOR:-nano}}"

usage() {
  cat <<'EOF'
Usage:
  hubos-runtime-setup.sh [interactive]
  hubos-runtime-setup.sh show
  hubos-runtime-setup.sh path
  hubos-runtime-setup.sh edit
  hubos-runtime-setup.sh enable PROFILE
  hubos-runtime-setup.sh disable PROFILE
  hubos-runtime-setup.sh set-default PROFILE
  hubos-runtime-setup.sh assign APP_ID PROFILE
  hubos-runtime-setup.sh unassign APP_ID
  hubos-runtime-setup.sh verify

Environment:
  HUBOS_RUNTIME_CONFIG  Override the runtime config path.
  EDITOR / VISUAL       Editor used by the edit subcommand.
EOF
}

run_python() {
  python3 - "$config_path" "$@" <<'PY'
import json
import pathlib
import sys

config_path = pathlib.Path(sys.argv[1])
args = sys.argv[2:]

if not config_path.is_file():
    raise SystemExit(f"Missing runtime config: {config_path}")

data = json.loads(config_path.read_text())

if data.get("schema_version") != 1:
    raise SystemExit(f"Unsupported schema_version in {config_path}")

catalog = data.get("runtime_catalog")
selection = data.get("selection")
if not isinstance(catalog, list) or not isinstance(selection, dict):
    raise SystemExit(f"Malformed runtime config: {config_path}")

profiles = {}
for entry in catalog:
    profile_id = entry.get("id")
    if not isinstance(profile_id, str) or not profile_id:
        raise SystemExit("Every runtime profile must have a non-empty string id")
    if profile_id in profiles:
        raise SystemExit(f"Duplicate runtime profile id: {profile_id}")
    profiles[profile_id] = entry

installed = selection.get("installed_profiles")
if not isinstance(installed, list):
    raise SystemExit("selection.installed_profiles must be a list")
for profile_id in installed:
    if profile_id not in profiles:
        raise SystemExit(f"Installed runtime profile is missing from runtime_catalog: {profile_id}")

default_profile = selection.get("default_profile")
if default_profile not in profiles:
    raise SystemExit(f"Default runtime profile is missing from runtime_catalog: {default_profile}")
if default_profile not in installed:
    raise SystemExit(f"Default runtime profile is not installed: {default_profile}")

assignments = selection.get("app_assignments")
if not isinstance(assignments, dict):
    raise SystemExit("selection.app_assignments must be an object")
for app_id, profile_id in assignments.items():
    if not isinstance(app_id, str) or not app_id:
        raise SystemExit("Every app assignment key must be a non-empty string")
    if profile_id not in profiles:
        raise SystemExit(f"App assignment references unknown runtime profile: {profile_id}")
    if profile_id not in installed:
        raise SystemExit(f"App assignment references runtime profile that is not installed: {profile_id}")

command = args[0] if args else "show"

def write_back() -> None:
    config_path.write_text(json.dumps(data, indent=2) + "\n")

def normalize_installed() -> None:
    seen = set()
    normalized = []
    for profile_id in installed:
        if profile_id not in seen:
            normalized.append(profile_id)
            seen.add(profile_id)
    installed[:] = normalized

def show() -> None:
    print(f"Config: {config_path}")
    print(f"Default profile: {default_profile}")
    print("Installed profiles:")
    for profile_id in installed:
        profile = profiles[profile_id]
        print(
            f"  - {profile_id}: class={profile.get('guest_class')} "
            f"os={profile.get('os_family')} desc={profile.get('description')}"
        )
    print("App assignments:")
    if not assignments:
        print("  (none)")
    else:
        for app_id in sorted(assignments):
            print(f"  - {app_id}: {assignments[app_id]}")

if command == "show":
    show()
elif command == "verify":
    print(f"Runtime config verified: {config_path}")
elif command == "enable":
    profile_id = args[1]
    if profile_id not in profiles:
        raise SystemExit(f"Unknown runtime profile: {profile_id}")
    installed.append(profile_id)
    normalize_installed()
    write_back()
    print(f"Enabled runtime profile: {profile_id}")
elif command == "disable":
    profile_id = args[1]
    if profile_id == default_profile:
        raise SystemExit(f"Cannot disable default runtime profile: {profile_id}")
    if profile_id not in installed:
        raise SystemExit(f"Runtime profile is not installed: {profile_id}")
    for app_id, assigned_profile in list(assignments.items()):
        if assigned_profile == profile_id:
            raise SystemExit(
                f"Cannot disable runtime profile {profile_id}; it is assigned to app {app_id}"
            )
    installed[:] = [entry for entry in installed if entry != profile_id]
    write_back()
    print(f"Disabled runtime profile: {profile_id}")
elif command == "set-default":
    profile_id = args[1]
    if profile_id not in profiles:
        raise SystemExit(f"Unknown runtime profile: {profile_id}")
    if profile_id not in installed:
        raise SystemExit(f"Runtime profile is not installed: {profile_id}")
    selection["default_profile"] = profile_id
    write_back()
    print(f"Set default runtime profile: {profile_id}")
elif command == "assign":
    app_id = args[1]
    profile_id = args[2]
    if profile_id not in profiles:
        raise SystemExit(f"Unknown runtime profile: {profile_id}")
    if profile_id not in installed:
        raise SystemExit(f"Runtime profile is not installed: {profile_id}")
    assignments[app_id] = profile_id
    write_back()
    print(f"Assigned app {app_id} to runtime profile: {profile_id}")
elif command == "unassign":
    app_id = args[1]
    if app_id in assignments:
        del assignments[app_id]
        write_back()
    print(f"Cleared runtime assignment for app: {app_id}")
elif command == "list-ids":
    for profile_id in profiles:
        print(profile_id)
else:
    raise SystemExit(f"Unsupported command: {command}")
PY
}

interactive_setup() {
  run_python show
  printf '\n'
  printf 'Available runtime profiles:\n'
  run_python list-ids | while IFS= read -r profile_id; do
    printf '  - %s\n' "$profile_id"
  done

  printf '\nInstalled profiles [%s]: ' "$(python3 - "$config_path" <<'PY'
import json
import pathlib
import sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
print(",".join(data["selection"]["installed_profiles"]))
PY
)"
  IFS= read -r installed_input || true
  if [ -n "${installed_input:-}" ]; then
    python3 - "$config_path" "$installed_input" <<'PY'
import json
import pathlib
import sys

config_path = pathlib.Path(sys.argv[1])
requested = [item.strip() for item in sys.argv[2].split(",") if item.strip()]
data = json.loads(config_path.read_text())
catalog_ids = {entry["id"] for entry in data["runtime_catalog"]}
for profile_id in requested:
    if profile_id not in catalog_ids:
        raise SystemExit(f"Unknown runtime profile: {profile_id}")
if not requested:
    raise SystemExit("At least one installed profile is required")
data["selection"]["installed_profiles"] = requested
if data["selection"]["default_profile"] not in requested:
    data["selection"]["default_profile"] = requested[0]
data["selection"]["app_assignments"] = {
    app_id: profile_id
    for app_id, profile_id in data["selection"]["app_assignments"].items()
    if profile_id in requested
}
config_path.write_text(json.dumps(data, indent=2) + "\n")
print(f"Installed profiles updated: {', '.join(requested)}")
PY
  fi

  printf 'Default runtime profile [%s]: ' "$(python3 - "$config_path" <<'PY'
import json
import pathlib
import sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
print(data["selection"]["default_profile"])
PY
)"
  IFS= read -r default_input || true
  if [ -n "${default_input:-}" ]; then
    run_python set-default "$default_input"
  fi

  printf 'App-specific runtime assignment (APP_ID PROFILE, blank to skip): '
  IFS= read -r assignment_input || true
  if [ -n "${assignment_input:-}" ]; then
    set -- $assignment_input
    if [ "$#" -ne 2 ]; then
      echo "Expected: APP_ID PROFILE" >&2
      exit 1
    fi
    run_python assign "$1" "$2"
  fi

  printf 'Open the config in %s for manual edits? [y/N]: ' "$editor"
  IFS= read -r edit_answer || true
  case "${edit_answer:-}" in
    y|Y|yes|YES)
      "$editor" "$config_path"
      ;;
  esac

  run_python verify
}

command="${1:-interactive}"

case "$command" in
  interactive)
    interactive_setup
    ;;
  show)
    run_python show
    ;;
  path)
    printf '%s\n' "$config_path"
    ;;
  edit)
    "$editor" "$config_path"
    run_python verify
    ;;
  enable|disable|set-default|unassign)
    [ "$#" -eq 2 ] || usage
    run_python "$1" "$2"
    ;;
  assign)
    [ "$#" -eq 3 ] || usage
    run_python "$1" "$2" "$3"
    ;;
  verify)
    run_python verify
    ;;
  --help|-h|help)
    usage
    ;;
  *)
    usage >&2
    exit 1
    ;;
esac
