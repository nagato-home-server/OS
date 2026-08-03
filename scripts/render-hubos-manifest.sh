#!/bin/sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
source_json="${1:-$repo_root/seL4test-manifest/source-boundaries.json}"
out_xml="${2:-$repo_root/seL4test-manifest/hubos.xml}"

python3 - "$source_json" "$out_xml" <<'PY'
import json
import pathlib
import sys
from xml.sax.saxutils import escape

source_json = pathlib.Path(sys.argv[1])
out_xml = pathlib.Path(sys.argv[2])

data = json.loads(source_json.read_text())
manifest = data["manifest"]
projects = data["native_projects"] + data.get("driver_projects", [])

lines = []
lines.append('<?xml version="1.0" encoding="UTF-8"?>')
lines.append('<!--')
lines.append('     Copyright 2026 HubOS')
lines.append('     SPDX-License-Identifier: BSD-2-Clause')
lines.append('     Generated from seL4test-manifest/source-boundaries.json')
lines.append('-->')
lines.append('<manifest>')
lines.append(f'  <remote name="{escape(manifest["remote_name"])}" fetch="{escape(manifest["fetch"])}"/>')
lines.append('')
lines.append(f'  <default remote="{escape(manifest["remote_name"])}" revision="{escape(manifest["default_revision"])}"/>')
lines.append('')

for project in projects:
    attrs = [
        f'name="{escape(project["name"])}"',
        f'path="{escape(project["path"])}"',
        f'revision="{escape(project["revision"])}"',
    ]
    linkfiles = project.get("linkfiles", [])
    if not linkfiles:
        lines.append(f'  <project {" ".join(attrs)}/>')
        continue

    lines.append(f'  <project {" ".join(attrs)}>')
    for linkfile in linkfiles:
        lines.append(
            f'    <linkfile src="{escape(linkfile["src"])}" dest="{escape(linkfile["dest"])}"/>'
        )
    lines.append('  </project>')

lines.append('</manifest>')

out_xml.write_text("\n".join(lines) + "\n")
PY

printf '%s\n' "Rendered HubOS manifest at $out_xml"
