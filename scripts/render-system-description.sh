#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
source_json="${HUBOS_SOURCE_BOUNDARIES:-$repo_root/seL4test-manifest/source-boundaries.json}"
target_board="${HUBOS_TARGET_BOARD:-}"
out_system=""
print_field=""

usage() {
  cat <<'EOF' >&2
Usage:
  render-system-description.sh [TARGET_BOARD] [OUT_SYSTEM]
  render-system-description.sh --print-field FIELD [TARGET_BOARD]

Fields:
  microkit_board
  system
EOF
  exit 1
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --print-field)
      shift
      [ "$#" -gt 0 ] || usage
      print_field="$1"
      ;;
    --source-json)
      shift
      [ "$#" -gt 0 ] || usage
      source_json="$1"
      ;;
    -*)
      usage
      ;;
    *)
      if [ -z "$target_board" ]; then
        target_board="$1"
      elif [ -z "$out_system" ] && [ -z "$print_field" ]; then
        out_system="$1"
      else
        usage
      fi
      ;;
  esac
  shift
done

python3 - "$source_json" "$target_board" "$out_system" "$print_field" "$repo_root" <<'PY'
import json
import pathlib
import sys
import xml.etree.ElementTree as ET

source_json = pathlib.Path(sys.argv[1])
requested_board = sys.argv[2]
out_system = sys.argv[3]
print_field = sys.argv[4]
repo_root = pathlib.Path(sys.argv[5])

data = json.loads(source_json.read_text())
board_targets = data.get("board_targets", {})
boards = board_targets.get("boards", [])
default_board = board_targets.get("default")
target_board = requested_board or default_board

if not target_board:
    raise SystemExit("No board target selected and no default board target is defined")

board = next((entry for entry in boards if entry.get("name") == target_board), None)
if board is None:
    raise SystemExit(f"Unknown board target: {target_board}")

known_driver_projects = {project["name"] for project in data.get("driver_projects", [])}
for project_name in board.get("driver_projects", []):
    if project_name not in known_driver_projects:
        raise SystemExit(
            f"Board target {target_board} references missing driver project: {project_name}"
        )

if print_field:
    if print_field not in board:
        raise SystemExit(f"Unknown board field for {target_board}: {print_field}")
    print(board[print_field])
    raise SystemExit(0)

if not out_system:
    raise SystemExit("Missing output path for rendered system description")

system_path = repo_root / board["system"]
if not system_path.is_file():
    raise SystemExit(f"Missing board system description: {system_path}")

component_service_macros = {
    "Root Task": "HUBOS_MICROKIT_COMPONENT_ROOT_TASK",
    "Resource Registry": "HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY",
    "Capability Manager": "HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER",
    "Session Manager": "HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER",
    "Memory Manager": "HUBOS_MICROKIT_COMPONENT_MEMORY_MANAGER",
    "DMA Manager": "HUBOS_MICROKIT_COMPONENT_DMA_MANAGER",
    "Hub": "HUBOS_MICROKIT_COMPONENT_HUB",
    "Driver Registry": "HUBOS_MICROKIT_COMPONENT_DRIVER_REGISTRY",
    "Driver Loader": "HUBOS_MICROKIT_COMPONENT_DRIVER_LOADER",
    "Bus Managers": "HUBOS_MICROKIT_COMPONENT_BUS_MANAGERS",
    "Driver Service": "HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE",
    "Network Server": "HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER",
    "Device Server": "HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER",
    "Storage Server": "HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER",
    "Display Server": "HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER",
}

component_dirs = {
    "Root Task": "root-task",
    "Resource Registry": "resource-registry",
    "Capability Manager": "capability-manager",
    "Session Manager": "session-manager",
    "Memory Manager": "memory-manager",
    "DMA Manager": "dma-manager",
    "Hub": "hub",
    "Driver Registry": "driver-registry",
    "Driver Loader": "driver-loader",
    "Bus Managers": "bus-managers",
    "Driver Service": "driver-service",
    "Network Server": "network-server",
    "Device Server": "device-server",
    "Storage Server": "storage-server",
    "Display Server": "display-server",
}

notification_receivers = {
    "DMA Manager",
    "Driver Loader",
    "Bus Managers",
    "Driver Service",
    "Network Server",
    "Device Server",
    "Storage Server",
    "Display Server",
}

dependencies = {
    "Resource Registry": ["Root Task"],
    "Capability Manager": ["Root Task", "Resource Registry"],
    "Session Manager": ["Root Task", "Capability Manager"],
    "Memory Manager": ["Root Task", "Resource Registry"],
    "DMA Manager": ["Root Task", "Memory Manager", "Resource Registry"],
    "Hub": ["Resource Registry", "Capability Manager", "Session Manager"],
    "Driver Registry": ["Root Task", "Resource Registry"],
    "Driver Loader": ["Root Task", "Driver Registry"],
    "Bus Managers": ["Root Task", "Resource Registry", "Driver Registry"],
    "Driver Service": ["Root Task", "Capability Manager", "Driver Registry", "Driver Loader"],
    "Network Server": ["Root Task", "Session Manager", "Hub", "Resource Registry"],
    "Device Server": ["Root Task", "Capability Manager", "Session Manager", "DMA Manager", "Bus Managers"],
    "Storage Server": ["Root Task", "Session Manager", "Hub", "Resource Registry"],
    "Display Server": ["Root Task", "Session Manager", "Hub", "Resource Registry"],
}

def collect_used_ids(root: ET.Element) -> dict[str, set[int]]:
    used: dict[str, set[int]] = {}
    for pd in root.findall("protection_domain"):
        pd_name = pd.get("name")
        if not pd_name:
            continue
        pd_used = used.setdefault(pd_name, set())
        for irq in pd.findall("irq"):
            irq_id = irq.get("id")
            if irq_id is not None:
                pd_used.add(int(irq_id, 0))
        for ioport in pd.findall("ioport"):
            ioport_id = ioport.get("id")
            if ioport_id is not None:
                pd_used.add(int(ioport_id, 0))
    for channel in root.findall("channel"):
        for end in channel.findall("end"):
            pd_name = end.get("pd")
            channel_id = end.get("id")
            if not pd_name or channel_id is None:
                continue
            used.setdefault(pd_name, set()).add(int(channel_id, 0))
    return used

def allocate_id(used_ids: set[int], reserved_id: int | None = None) -> int:
    for candidate in range(61, -1, -1):
        if candidate in used_ids:
            continue
        if reserved_id is not None and candidate == reserved_id:
            continue
        used_ids.add(candidate)
        return candidate
    raise SystemExit("Exhausted Microkit channel IDs while generating HubOS system channels")

def ensure_endpoint_channels(root: ET.Element) -> None:
    present_pds = {
        pd.get("name")
        for pd in root.findall("protection_domain")
        if pd.get("name")
    }
    used_ids = collect_used_ids(root)
    existing_channels: set[tuple[str, int, str, int, bool]] = set()
    existing_endpoint_pairs: set[tuple[str, str]] = set()
    existing_notification_pairs: set[tuple[str, str]] = set()
    for channel in root.findall("channel"):
        ends = channel.findall("end")
        if len(ends) != 2:
            continue
        a_pd = ends[0].get("pd")
        b_pd = ends[1].get("pd")
        a_id = ends[0].get("id")
        b_id = ends[1].get("id")
        if not a_pd or not b_pd or a_id is None or b_id is None:
            continue
        existing_channels.add(
            (a_pd, int(a_id, 0), b_pd, int(b_id, 0), ends[0].get("pp") == "true")
        )
        existing_channels.add(
            (b_pd, int(b_id, 0), a_pd, int(a_id, 0), ends[1].get("pp") == "true")
        )
        if ends[0].get("pp") == "true" or ends[1].get("pp") == "true":
            if ends[1].get("pp") == "true":
                existing_endpoint_pairs.add((b_pd, a_pd))
            if ends[0].get("pp") == "true":
                existing_endpoint_pairs.add((a_pd, b_pd))
        else:
            existing_notification_pairs.add((a_pd, b_pd))
            existing_notification_pairs.add((b_pd, a_pd))

    for component_name, component_dependencies in dependencies.items():
        if component_name not in present_pds:
            continue
        component_used = used_ids.setdefault(component_name, set())

        for dependency_name in component_dependencies:
            if dependency_name not in present_pds:
                continue
            if dependency_name in component_service_macros:
                if (component_name, dependency_name) not in existing_endpoint_pairs:
                    dependency_used = used_ids.setdefault(dependency_name, set())
                    client_id = allocate_id(component_used)
                    server_id = allocate_id(dependency_used)
                    channel = ET.SubElement(root, "channel")
                    ET.SubElement(channel, "end", pd=dependency_name, id=str(server_id))
                    ET.SubElement(channel,
                                  "end",
                                  pd=component_name,
                                  id=str(client_id),
                                  pp="true")
                    existing_channels.add((component_name, client_id, dependency_name, server_id, True))
                    existing_channels.add((dependency_name, server_id, component_name, client_id, False))
                    existing_endpoint_pairs.add((component_name, dependency_name))

            if component_name in notification_receivers:
                if (dependency_name, component_name) not in existing_notification_pairs:
                    dependency_used = used_ids.setdefault(dependency_name, set())
                    sender_id = allocate_id(dependency_used)
                    receiver_id = allocate_id(component_used)
                    channel = ET.SubElement(root, "channel")
                    ET.SubElement(channel, "end", pd=dependency_name, id=str(sender_id))
                    ET.SubElement(channel, "end", pd=component_name, id=str(receiver_id))
                    existing_channels.add((dependency_name, sender_id, component_name, receiver_id, False))
                    existing_channels.add((component_name, receiver_id, dependency_name, sender_id, False))
                    existing_notification_pairs.add((dependency_name, component_name))
                    existing_notification_pairs.add((component_name, dependency_name))

def write_channel_map_headers(root: ET.Element, system_output: pathlib.Path) -> None:
    generated_root = system_output.parent / "generated"
    if not generated_root.is_dir():
        return

    present_pds = {
        pd.get("name")
        for pd in root.findall("protection_domain")
        if pd.get("name")
    }
    endpoint_handshakes: dict[str, tuple[int, str, str]] = {}
    incoming_endpoints: dict[str, list[int]] = {}
    notify_handshakes: dict[str, tuple[int, str]] = {}
    incoming_notifications: dict[str, list[int]] = {}

    for channel in root.findall("channel"):
        ends = channel.findall("end")
        if len(ends) != 2:
            continue

        a_pd = ends[0].get("pd")
        b_pd = ends[1].get("pd")
        a_id = ends[0].get("id")
        b_id = ends[1].get("id")
        a_pp = ends[0].get("pp") == "true"
        b_pp = ends[1].get("pp") == "true"
        if not a_pd or not b_pd or a_id is None or b_id is None:
            continue

        if b_pp and a_pd in component_service_macros:
            endpoint_handshakes.setdefault(b_pd, (int(b_id, 0), component_service_macros[a_pd], a_pd))
            incoming_endpoints.setdefault(a_pd, []).append(int(a_id, 0))
        if a_pp and b_pd in component_service_macros:
            endpoint_handshakes.setdefault(a_pd, (int(a_id, 0), component_service_macros[b_pd], b_pd))
            incoming_endpoints.setdefault(b_pd, []).append(int(b_id, 0))

        if not a_pp and not b_pp:
            if b_pd in notification_receivers and a_pd in dependencies.get(b_pd, []):
                notify_handshakes.setdefault(a_pd, (int(a_id, 0), b_pd))
                incoming_notifications.setdefault(b_pd, []).append(int(b_id, 0))
            if a_pd in notification_receivers and b_pd in dependencies.get(a_pd, []):
                notify_handshakes.setdefault(b_pd, (int(b_id, 0), a_pd))
                incoming_notifications.setdefault(a_pd, []).append(int(a_id, 0))

    for component_name, component_dir in component_dirs.items():
        header_path = generated_root / component_dir / "channel-map.h"
        if component_name not in present_pds:
            endpoint = None
            notify = None
            endpoint_channels: list[int] = []
            notify_channels: list[int] = []
        else:
            endpoint = endpoint_handshakes.get(component_name)
            notify = notify_handshakes.get(component_name)
            endpoint_channels = incoming_endpoints.get(component_name, [])
            notify_channels = incoming_notifications.get(component_name, [])
        header_path.write_text(
            "\n".join(
                [
                    "#ifndef HUBOS_GENERATED_CHANNEL_MAP_H",
                    "#define HUBOS_GENERATED_CHANNEL_MAP_H",
                    "",
                    (
                        "static const microkit_channel hubos_generated_incoming_endpoint_channels[] = {" +
                        ", ".join(str(channel_id) for channel_id in endpoint_channels) +
                        "};"
                    ) if endpoint_channels else
                    "static const microkit_channel hubos_generated_incoming_endpoint_channels[1] = {0};",
                    (
                        "static const microkit_channel hubos_generated_incoming_notification_channels[] = {" +
                        ", ".join(str(channel_id) for channel_id in notify_channels) +
                        "};"
                    ) if notify_channels else
                    "static const microkit_channel hubos_generated_incoming_notification_channels[1] = {0};",
                    "",
                    f"#define HUBOS_GENERATED_INCOMING_ENDPOINT_COUNT {len(endpoint_channels)}",
                    f"#define HUBOS_GENERATED_INCOMING_NOTIFICATION_COUNT {len(notify_channels)}",
                    "",
                    f"#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_ENABLED {1 if endpoint else 0}",
                    f"#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_CHANNEL {endpoint[0] if endpoint else 0}",
                    (
                        f"#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_SERVICE {endpoint[1]}"
                        if endpoint
                        else "#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_SERVICE HUBOS_MICROKIT_COMPONENT_ROOT_TASK"
                    ),
                    (
                        f'#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_NAME "{endpoint[2]}"'
                        if endpoint
                        else '#define HUBOS_GENERATED_ENDPOINT_HANDSHAKE_NAME "none"'
                    ),
                    "",
                    f"#define HUBOS_GENERATED_NOTIFY_HANDSHAKE_ENABLED {1 if notify else 0}",
                    f"#define HUBOS_GENERATED_NOTIFY_HANDSHAKE_CHANNEL {notify[0] if notify else 0}",
                    (
                        f'#define HUBOS_GENERATED_NOTIFY_HANDSHAKE_NAME "{notify[1]}"'
                        if notify
                        else '#define HUBOS_GENERATED_NOTIFY_HANDSHAKE_NAME "none"'
                    ),
                    "",
                    "#endif",
                    "",
                ]
            )
        )

out_path = pathlib.Path(out_system)
out_path.parent.mkdir(parents=True, exist_ok=True)
tree = ET.parse(system_path)
root = tree.getroot()
if root.tag != "system":
    raise SystemExit(f"Unexpected root element in system description: {root.tag}")
ensure_endpoint_channels(root)
write_channel_map_headers(root, out_path)
ET.indent(tree, space="  ")
tree.write(out_path, encoding="UTF-8", xml_declaration=True)
print(f"Rendered {target_board} system description at {out_path}")
PY
