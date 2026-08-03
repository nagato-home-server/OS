# seL4 Workspace Scaffold

This repository keeps the seL4 integration pinned and reproducible without
mixing the kernel tree into the host-side model.

## What is included

- `seL4test-manifest/hubos.xml`
- `seL4test-manifest/source-boundaries.json`
- `scripts/bootstrap-sel4-workspace.sh`
- `scripts/bootstrap-qemu-workspace.sh`
- `docs/sel4-integration-notes.md`
- `docs/source-boundaries.md`

## What the scaffold does

- copies the pinned manifest into a local workspace directory
- leaves the kernel, tools, and user-space projects to upstream seL4 sources
- keeps the HubOS model separate from the seL4 workspace layout
- renders Microkit-shaped component source stubs from the host-side manifest
- emits per-component metadata files alongside the stubs so the generated
  workspace can be rebuilt from the host-side service graph

## Why this shape

- seL4 is the minimal kernel boundary
- the Root Task owns the user-space trust base
- resource registration, capability policy, session trees, and hub resolution
  stay in user space
- driver loading and network policy are split into restartable services

## Implementation Target

- host-side model first
- Microkit-first for the seL4 user-space layout
- CPIO / service manifests / boot graph after the service split is stable
- service graph stays host-side until the seL4 wiring is introduced
- generated Microkit sources can be rendered before handoff to the SDK
- the generated-workspace renderer also accepts an imported upstream-generated
  tree via `HUBOS_MICROKIT_GENERATED_SOURCE` when you have real tool output to
  drop in
- source boundary summaries are emitted alongside the generated workspace
- the bootstrap step stages the local `src/hubos-upstream/` mirror into
  `upstream-mirror/` for review and offline inspection
- board-specific driver repos can be added through `driver_projects` in
  `seL4test-manifest/source-boundaries.json` and will be included in the pinned
  manifest and local staging flow
- board-specific `.system` files are selected through `board_targets`; set
  `HUBOS_TARGET_BOARD` to switch the rendered system description

## Official upstream sources

The manifest pins only public GitHub repositories from the seL4 ecosystem and
related dependencies:

- `seL4/seL4.git`
- `seL4/seL4_tools.git`
- `seL4/seL4_libs.git`
- `seL4/util_libs.git`
- `seL4/sel4runtime.git`
- `seL4/sel4test.git`
- `seL4/sel4_projects_libs.git`
- `seL4/musllibc.git`
- `seL4/microkit.git`
- `seL4/camkes-tool.git`
- `seL4/sel4bench.git`
- `lwip-tcpip/lwip.git`
- `riscv/opensbi.git`
- `nanopb/nanopb.git`

Candidate reuse assets and the rationale for each choice are tracked in
[`docs/reuse-assets.md`](/home/nagatoyuki/Downloads/OS/docs/reuse-assets.md).
The native-vs-optional split is fixed in
[`docs/source-boundaries.md`](/home/nagatoyuki/Downloads/OS/docs/source-boundaries.md).

## Runbook

Use the bootstrap script to stage a workspace:

```sh
./scripts/bootstrap-sel4-workspace.sh
```

Then initialize the upstream workspace with the pinned manifest and the staged
local mirror.

To regenerate the manifest from the source boundary JSON, run:

```sh
./scripts/render-hubos-manifest.sh
```

To render the component stubs, or import an upstream-generated tree via
`HUBOS_MICROKIT_GENERATED_SOURCE`, run:

```sh
./scripts/render-microkit-generated.sh
```

To verify the local seL4 integration scaffold without fetching external
sources, run:

```sh
./scripts/verify-sel4-integration.sh
```

To prepare a QEMU-oriented workspace scaffold and launch wrapper, run:

```sh
./scripts/bootstrap-qemu-workspace.sh
./scripts/verify-qemu-scaffold.sh
```

To render the boot package seam for an externally built system image, run:

```sh
./scripts/render-boot-package.sh
./scripts/verify-boot-package.sh
```

To verify the native upstream asset set together with the scaffold, run:

```sh
cmake --build build --target hubos_full_verify
```

To verify the optional Linux VM backend asset set, run:

```sh
cmake --build build --target hubos_full_vm_verify
```
