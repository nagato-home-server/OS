# seL4 Integration Notes

This repository models a Hub-oriented Session OS on top of seL4.

The integration constraint is straightforward:

- seL4 stays the minimal kernel
- policy remains in user space
- device ownership stays in dedicated servers
- names remain identification only
- capabilities remain the only authorization token

## Observed seL4 Shape

The current implementation matches the seL4 split in these ways:

- the kernel boundary is minimal
- the root task owns the bootstrap trust base and publishes service endpoints
- the root task is bootstrap-only after initial service publication
- sessions are a user-space contract, not a kernel concept
- driver lifecycle must be handled in a restartable user-space service
- DMA must be mediated by a dedicated manager
- memory, DMA, driver registry, loader, and bus management stay in user space
- IPC is modeled as per-service endpoints after bootstrap
- the runtime stub and endpoint handlers are repository-rendered callbacks, but their badge routing matches the upstream Microkit shape
- the generated callback surface already runs under the upstream Microkit / `libmicrokit` event loop in QEMU; the remaining work is replacing the rendered sources with SDK-generated output
- the Microkit render script can now import an upstream-generated workspace tree when one is available, which gives the repository a concrete replacement seam instead of only a local renderer
- the boot manifest tracks which components publish endpoints, notifications, IRQs, and shared memory
- driver trust and driver binding are split into separate services
- network namespaces, routing, and NIC selection are owned by a network server
- the local `src/hubos-upstream/` mirror is used for source selection and
  license confirmation, not as the build source of record
- generated workspace output now includes a machine-readable source boundary
  summary alongside the Microkit stubs
- the Microkit service graph is the current seL4 layout reference
- the local repository can verify the scaffolded seL4 integration without
  needing a kernel fetch
- the boot package seam can attach a built image directly via
  `HUBOS_BOOT_IMAGE` or resolve one from `HUBOS_BOOT_IMAGE_BUILD_DIR`
- the machine-readable source boundary manifest drives both manifest generation
  and local upstream staging
- the network server can switch its default backend at build time between
  lwIP and the optional Linux VM backend

## Command Surface Needed For Tree Management

The current control plane needs explicit commands to preserve the session tree
and keep Root Task bootstrap-only. The seL4-facing vocabulary should be grouped
by ownership domain instead of routed through an ad hoc Root Task dispatcher.

Required command families:

- bootstrap: `complete_boot_step`, `boot_step_is_complete`
- resource: `register`, `update_state`, `quarantine`, `retire`, `describe`
- capability: `issue`, `copy`, `mint_from`, `transfer`, `revoke`,
  `authorize`
- session: `create`, `refresh_context`, `set_state`, `is_ancestor`,
  `child_count`, `revoke_tree`
- hub: `resolve`, `authorize`
- driver: `bind`, `rebind`, `quarantine`, `unbind`
- network: `bind_namespace`, `set_policy`, `add_route`, `set_default_route`,
  `select_nic`, `bind_port`, `set_failover_policy`, `describe`
- storage: `bind_namespace`, `release_namespace`, `finalize_namespace`,
  `describe`
- display: `bind_namespace`, `release_namespace`, `finalize_namespace`,
  `describe`
- device: `set_owner`, `release_owner`, `reset`, `quarantine`,
  `clear_quarantine`, `attach_mmio`, `attach_irq`, `attach_dma`, `describe`
- VM control: `set_guest_memory`, `set_vcpu_count`, `attach_virtio_net`,
  `attach_virtio_blk`, `attach_vgpu`, `set_artifacts`, `start`, `describe`
- app: `describe`, `describe_structure`, `set_kind`, `set_envelope`,
  `bind_session`, `bind_namespace`, `start`, `stop`, `reset`

The VM/app control family is the most important missing piece if the intended
flow is:

1. boot seL4
2. land in the seL4 console
3. let Hub prepare the app placement
4. start Linux or another app from the seL4 control plane
5. manage the app through explicit control-plane commands

Without a `start` command, the VM configuration is only a data model and does
not yet give the microkernel side a first-class way to launch the guest.

For multi-app setups, the command target should be `app_<n>`:

- `app_1 describe_structure`
- `app_1 describe`
- `app_1 start`
- `app_2 describe_structure`
- `app_2 start`

For small apps, the preferred sequence is often two-step:

1. `hub describe` or `hub resolve`
2. `app_<n> start`

That keeps placement, ownership, and launch separate.

## Trusted GitHub Dependencies

The integration manifest pins the following public repositories:

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

The driver/network side is intentionally kept to public, upstream repositories. The public `sddf` repository could not be verified from this environment, so it is not pinned here.
The optional Linux VM backend remains separate from the native manifest and is
tracked as an upstream asset, not as part of the native seL4 control plane.

## Compatibility Strategy

The codebase should be mapped into these seL4 user-space roles:

- `Root Task` -> registry and policy coordinator
- `Resource Registry` -> canonical resource ledger
- `Capability Manager` -> authorization policy on top of kernel caps
- `Session Manager` -> session tree and cascade revoke
- `Hub` -> name resolution and policy dispatch only
- `DMA Manager` -> explicit DMA authorization and revoke
- `Driver Loader` -> restartable trust verification and key rotation
- `Driver Service` -> restartable binding, rebind, and quarantine
- `Network Server` -> lwIP-backed namespace, routing, and NIC selection service
  on native seL4; Linux `NetworkManager` is an optional backend only when the
  network plane is hosted inside a Linux VM
- `Device Server` -> hardware ownership and MMIO/IRQ handling with explicit attachment claims
- `Storage Server` and `Display Server` -> namespace ownership for block and graphics services
- `Container / VM / App` -> consumer-side session owners
- `Root Task` -> bootstrap-only endpoint publication and initial capability
  distribution, not steady-state policy dispatch

This is the shape the current host-side model is already converging toward.

## Kernel Caveats That Affect This Design

The kernel documentation makes the following operational constraints explicit:

- verified support is platform/configuration-specific
- MCS and SMP are not the same stability class as the base verified configurations
- VT-d / IOMMU behavior is platform-specific and may need to be disabled on some systems
- reused address spaces require explicit cleanup of old frame capabilities

These caveats reinforce the current model choices:

- DMA revocation must be explicit and fail-safe
- session and capability teardown must be cascade-aware
- VM and container lifecycle code should not assume dynamic kernel cleanup
