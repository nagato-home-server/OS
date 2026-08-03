# Source Boundaries

This document fixes where HubOS owns code and where upstream dependencies are
consumed.

The machine-readable source of truth is
[`seL4test-manifest/source-boundaries.json`](/home/nagatoyuki/Downloads/OS/seL4test-manifest/source-boundaries.json).
`seL4test-manifest/hubos.xml` is generated from it.

## Native seL4 Path

The native path is the default and is expected to stay GPL-2.0-compatible at
the kernel-adjacent boundary.

Mandatory upstream sources:

- `seL4/seL4.git`
- `seL4/microkit.git`
- `seL4/sel4runtime.git`
- `seL4/seL4_libs.git`
- `seL4/util_libs.git`
- `seL4/musllibc.git`
- `seL4/seL4_tools.git`
- `seL4/sel4_projects_libs.git`
- `seL4/sel4test.git`
- `seL4/camkes-tool.git`
- `seL4/sel4bench.git`
- `lwip-tcpip/lwip.git`
- `riscv/opensbi.git`
- `nanopb/nanopb.git`

## Optional Linux VM Backend

The only optional backend currently tracked is:

- `NetworkManager/NetworkManager`

This backend is only relevant when the network plane runs inside a Linux VM.
It must stay behind a boundary seam so the native seL4 path does not depend on
it.

## Board Driver Repositories

Hardware-specific driver or board-support repositories can be added through the
`driver_projects` section of `seL4test-manifest/source-boundaries.json`.

Use this when the target board needs one or more upstream git checkouts for:

- device tree helpers
- board support packages
- MMIO/IRQ tables
- vendor driver code that should stay outside the core HubOS tree

These repositories are treated as first-class source-boundary inputs and are
rendered into the pinned manifest and local upstream staging flow alongside the
native seL4 dependencies.

## Board Targets

Board selection is tracked separately from the raw repository list in the
`board_targets` section of
[`seL4test-manifest/source-boundaries.json`](/home/nagatoyuki/Downloads/OS/seL4test-manifest/source-boundaries.json).

Each board target should define:

- `name`: the HubOS board identifier
- `microkit_board`: the upstream Microkit board name used for builds
- `system`: the checked-in board-specific `.system` file
- `driver_projects`: the subset of `driver_projects` that belong to that board

The current default target is `qemu-x86_64_generic`, which maps to the checked-in
QEMU system description under
[`boards/qemu-x86_64_generic/hubos.system`](/home/nagatoyuki/Downloads/OS/boards/qemu-x86_64_generic/hubos.system).

When adding real hardware, put the driver or BSP repositories in
`driver_projects`, add a board target that references them, and then place the
board-specific cap / map / irq wiring in that board's `.system` file.

## HubOS-Owned Code

HubOS owns the following code and should wrap upstream assets around these
boundaries instead of inlining upstream behavior directly:

- resource registry
- capability manager
- session manager
- hub / name resolution
- driver registry
- driver loader trust chain
- driver service lifecycle control
- network server policy and backend selection
- root-task bootstrap orchestration
- Microkit IPC layout and runtime shims
- generated Microkit component stubs
- boot and service manifests

## Integration Points

- `seL4test-manifest/hubos.xml` is the native seL4 workspace manifest.
- `src/hubos-upstream/` is the local mirror of upstream dependencies used for
  review and source selection.
- `scripts/verify-upstream-assets.sh` checks mandatory native assets and the
  optional VM backend.
- `scripts/render-hubos-manifest.sh` generates `hubos.xml` from the JSON source
  boundary manifest.
- `scripts/stage-local-upstreams.sh` stages the local mirror into the workspace
  layout for inspection.
- `scripts/render-microkit-generated.sh` emits the generated workspace and the
  machine-readable source boundary summary.
- `CMakeLists.txt` wires the upstream checks into explicit verification targets.

## Rule of Thumb

If a dependency changes the trusted control plane, it belongs behind a HubOS
wrapper. If it only supplies a backend implementation behind a process or VM
boundary, it can remain optional.
