# Spec Gap Analysis

This document compares `README/spec.md` with the current repository state and
separates the implementation into three buckets:

- implemented
- partial
- missing

The goal is to keep the codebase aligned with the spec while making the
remaining divergence explicit.

For a prioritized action list, see `docs/todo-priority-checklist.md`.

## Implemented

- Resource Registry, including canonical-name deduplication, Resource ID
  issuance, state transitions, quarantine, and retirement; see
  `src/resource_registry.c` and `include/hubos/resource_registry.h`.
- Capability Manager, including issue, copy, mint, transfer, revoke, authorize,
  and revoke-by-owner helpers; see `src/capability_manager.c` and
  `include/hubos/capability_manager.h`.
- Session Manager, including tree structure, session creation, snapshot version
  inheritance on creation, child lookup, and cascade revoke; see
  `src/session_manager.c` and `include/hubos/session_manager.h`.
- Hub split from authorization, where name resolution is separate from
  capability checks; see `src/hub.c`, `README/spec.md`, and
  `include/hubos/hub.h`.
- DMA lifecycle modeling, including `ABORTED` as a terminal failure state and
  explicit revoke conditions; see `src/dma_manager.c` and
  `include/hubos/dma_manager.h`.
- Driver Registry, Driver Loader, and Driver Service split, including trust
  metadata, package validation, sandboxed rebind flow, quarantine, and audit
  hooks; see `src/driver_registry.c`, `src/driver_loader.c`, and
  `src/driver_service.c`.
- Network Server policy model, including namespace ownership, routing table,
  NIC selection, default route policy, firewall policy, and backend selection;
  see `src/network_server.c` and `include/hubos/network_server.h`.
- Microkit service graph, IPC layout, runtime stub, and generated-code shims;
  see `src/microkit_graph.c`, `src/microkit_ipc.c`, `src/microkit_runtime.c`,
  and `src/microkit_generated.c`. The generated transport path now uses the
  shared frame helpers and service-specific request/response marshalling, while
  the host-side generated dispatch surface routes through a dedicated
  kernel-glue shim to expose the protected-call message-register envelope and
  notification callback surface. The generated workspace stubs stay
  standalone and are driven by the upstream Microkit event loop; the raw
  protected-call receive/dispatch bridge lives in `src/microkit_kernel_glue.c`
  for the host-side model and tests.
- The host-side `hubos_model_test` currently passes against the checked-in
  build tree, which validates the generated transport surface and kernel-glue
  entrypoints that the docs above describe.
- Microkit entrypoint facade for bootstrap, root-task dispatch, service dispatch,
  and badge-based routing; see `include/hubos/microkit_generated.h` and
  `src/microkit_generated.c`.
- seL4 workspace bootstrap support, source-boundary manifest generation, and
  local upstream staging; see `seL4test-manifest/source-boundaries.json`,
  `scripts/render-hubos-manifest.sh`, `scripts/stage-local-upstreams.sh`, and
  `scripts/bootstrap-sel4-workspace.sh`.
- Native-versus-optional backend split for networking; see
  `CMakeLists.txt`, `src/network_server.c`, and
  `seL4test-manifest/source-boundaries.json`.

## Partial

- Root Task now exposes explicit boot-step control commands in the model, but
  the repository still renders its own Microkit-shaped callback sources rather
  than consuming SDK-generated workspace output; see `src/root_task.c`,
  `src/microkit_runtime.c`, and `src/microkit_generated.c`. The generated
  Microkit stubs still round-trip the transport frame through the shared
  helpers and synthesize service-specific responses, but the actual
  upstream-generated workspace is still missing from the tree even though the
  render script now accepts an imported upstream-generated source directory as
  a replacement input.
- QEMU-oriented workspace bootstrap, boot package seam, and launch wrapper
  scaffolding now exist, the boot package can attach an externally built
  system image from `HUBOS_BOOT_IMAGE` or scan `HUBOS_BOOT_IMAGE_BUILD_DIR`,
  and the repository now has a local build path for the bootable HubOS image.
- Session inheritance is represented by immutable version fields, but the full
  policy object model for quota, namespace view, policy context, and metadata is
  still coarse-grained; see `src/session_manager.c` and `README/spec.md`.
- Shared-resource finalization now has a generic model helper, and namespace
  handles carry lifecycle metadata with bind, release, and finalize helpers.
  The abstraction is now wired into the network, storage, and display server
  models, and storage/display now also have endpoint wrappers for the control
  plane, including system-level binding, release, finalize, and IPC dispatch,
  but it still needs broader system and real hardware integration for the full
  control-plane path.
- Device Server now exposes endpoint wrappers for quarantine, clearing
  quarantine, reset, and MMIO / IRQ / DMA ownership attachment, and the model
  now tracks explicit owner/session state plus per-interface attachment claims.
  The Microkit transport and dispatch surface now also expose the device
  control plane on badge 12, but it still lacks actual hardware-backed
  ownership and interrupt delivery.
- Driver trust chain checks now use SHA-256 package hashing plus deterministic
  key rotation and revocation policy, but the signature / keyring model is
  still host-side policy machinery rather than a real asymmetric trust root;
  see `src/driver_loader.c` and `README/spec.md` sections 12.5 and 29.5.
- Service descriptors are discovery metadata only. The Hub can resolve names,
  but the descriptor still needs an external Capability for actual access; see
  `include/hubos/hub.h` and `src/hub.c`.
- Storage and display namespaces are represented as handles, and the model now
  includes bind / release / finalize / describe helpers plus IPC dispatch, but
  there is not yet a hardware-backed Storage Server or Display Server lifecycle
  control plane; see `include/hubos/storage_server.h`,
  `include/hubos/display_server.h`, and `include/hubos/container_model.h`.
- Audit logging exists as an in-memory model, not as a durable or kernel-backed
  audit subsystem; see `src/audit.c`.
- The boot capability set is explicit and minimal in the model, but it is still
  a host-side trust-base representation rather than a real firmware-fed
  capability broker; see `src/boot.c` and `src/system.c`.

## Missing

- Actual upstream-generated Microkit workspace output, including real endpoint
  handlers, notifications, and kernel-backed IPC wiring. The current host-side
  model and generated stubs now expose a dedicated Root Task endpoint shape
  plus a concrete transport frame envelope with service-specific request and
  response marshalling, and they route through a dedicated kernel-glue shim
  with endpoint and notification admission checks. The QEMU path already
  boots through the upstream Microkit event loop in `libmicrokit`; what is
  still missing is replacing the repository-rendered stubs with tool-emitted
  workspace sources.
- Full kernel-side IPC wiring in the actual generated workspace, not image
  generation and QEMU bootability.
- Real driver signature verification, hash verification, and keyring rotation
  against a cryptographic trust root.
- Full device-server ownership path for MMIO, IRQ, reset, and device lifecycle
  tied to hardware-backed interrupts and MMIO claims.
- Runtime packet payload movement in the network data plane.
- Full Storage Server and Display Server lifecycle control-plane integration.
- End-to-end validation against a real seL4/Microkit workspace.

## Current Intentional Divergence

The repository intentionally splits the conceptual Hub into three separate
roles:

- Hub: name resolution and service discovery
- Capability Manager: authorization
- Session Manager: session tree management and lifecycle coordination
- Root Task: bootstrap publication plus explicit boot-step control commands

This is a deliberate implementation choice and is now documented in
`README/spec.md`.

## Priority Order For The Next Pass

1. Wire the actual seL4/Microkit entrypoints and generated runtime surface.
2. Wire the shared-resource finalization helper into remaining shared-memory
   ownership paths.
3. Flesh out device-server hardware ownership and reset lifecycle.
4. Close the driver trust-chain gap with cryptographic package validation.
5. Tighten the kernel-side IPC wiring and deployable control-plane semantics.
