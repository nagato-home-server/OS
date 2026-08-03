# Implementation Plan

This plan covers the remaining work needed to move the current host-side model
into a seL4-first, Microkit-oriented system.

## Current Baseline

Already in place:

- resource, capability, session, DMA, hub, audit, boot, memory, bus models
- trust-only driver loader
- split driver service
- service endpoint layer for registry, capability, session, hub, driver, and network
- Root Task bootstrap plus explicit boot-step control commands
- system-level wrappers for microkit graph, boot state, driver loader, and network policy
- Microkit IPC layout and service-endpoint badge mapping
- host-side Microkit runtime stub and badge-based endpoint handlers
- generated-code shim with Microkit callback surface, badge routing, and
  per-component generated metadata
- Microkit boot manifest with startup order and channel metadata
- session context snapshots for namespace view and policy context inheritance
- network server with namespace, routing, NIC selection, port binding, and failover policy
- expanded Microkit service graph reference including memory, DMA, driver registry, loader, and bus managers
- pinned official seL4 GitHub dependency manifest

## Priority 0: Freeze the Service Boundary

Goal:

- keep Root Task bootstrap explicit, while exposing boot-step control commands
- make steady-state control flow go through dedicated endpoints
- stop adding new policy into Root Task

Work items:

- finalize endpoint API shape for registry, capability, session, hub, driver, and network
- define what stays in Root Task during initial capability distribution only
- keep bootstrap state explicit and dormant after publication
- render the generated-code workspace from the host-side manifest

 Done when:

- Root Task has no ad hoc steady-state orchestration logic
- each core service has its own endpoint-facing API
- the service split is reflected in docs and code

## Priority 1: Microkit Service Graph

Goal:

- choose the static seL4 layout before adding kernel-specific code
- map the current model onto deployable Microkit components

Work items:

- define component boundaries for Root Task, Registry, Capability Manager, Session Manager, Hub, Driver Service, Network Server, and Device Servers
- define boot ordering and dependency edges
- define which components can restart independently

Done when:

- a Microkit service graph exists in the repo
- boot order is acyclic and documented
- every service has an owner and a restart story
- the service graph is available as a host-side model and reference doc

## Priority 2: seL4 IPC Wiring

Goal:

- replace model-only dispatch with real service endpoints
- keep authorization separate from name resolution

Work items:

- map each service endpoint to an seL4 endpoint / notification pattern
- define request and response marshalling for each service
- keep Hub as name resolution only
- keep Capability Manager as authorization only
- keep the host-side runtime stub aligned with the eventual generated Microkit entrypoints
- define the missing VM control commands (`set_guest_memory`,
  `set_vcpu_count`, `attach_virtio_net`, `attach_virtio_blk`,
  `attach_vgpu`, `set_artifacts`, `start`) so Linux is launched from the seL4
  side instead of from a standalone guest launcher
- keep tree-preserving commands explicit (`create`, `refresh_context`,
  `child_count`, `is_ancestor`, `revoke_tree`) so session management stays
  inspectable and reversible
- define app-level commands for `app_<n>` targets (`describe`,
  `describe_structure`, `set_kind`, `set_envelope`, `bind_session`,
  `bind_namespace`, `start`, `stop`, `reset`) so Linux and non-Linux apps can
  be handled uniformly
- keep Hub as the placement / resolution step before app launch when a small
  app needs a two-stage path
- make the x86-64 boot path use an explicit seL4 kernel ELF plus initrd so the
  QEMU launcher matches the actual boot artifacts

Done when:

- host-side request/response structs have a direct seL4 mapping
- service calls no longer rely on a single Root Task dispatch path
- name lookup and authorization remain distinct code paths
- runtime stub and endpoint handler shapes are documented before generated Microkit code is added
- generated code can be materialized into a workspace from repository templates
- Linux VM lifecycle can be expressed as explicit control-plane commands from
  the seL4 side
- app lifecycle can be inspected and launched by `app_<n>` identifier
- App VM runtimes can be selected through a checked-in HubOS config and an
  operator-facing setup command rather than a hard-coded guest OS choice

## Priority 3: Driver and Device Lifecycle

Goal:

- make driver ownership and hardware ownership restartable
- keep loader trust checks separate from active device binding

Work items:

- add the device-server side of MMIO / IRQ ownership
- connect Driver Service to driver lifecycle state transitions
- model quarantine, rebind, and recovery more explicitly

Done when:

- Driver Loader only answers "can this run?"
- Driver Service only answers "should this device be owned now?"
- device ownership is visible outside the Root Task

## Priority 4: Network Data Plane Policy

Goal:

- keep packet payload out of the control plane
- make routing and NIC choice policy-driven

Work items:

- preserve Network Server ownership of namespace, routing table, default route, NIC selection, firewall policy, port binding, and failover policy
- connect Network Session and NIC Session concepts to the service graph
- define the boundary between policy decisions and packet movement

Done when:

- Network Server decides policy, not packet forwarding
- data plane ownership is outside the control plane
- routing and NIC selection are explicit, testable policies

## Priority 5: Image and Boot Packaging

Goal:

- make the system bootable as a concrete artifact

Work items:

- define the CPIO image layout
- define service manifests and component startup metadata
- wire the boot graph into the chosen Microkit layout

Done when:

- the build produces a bootable image layout
- service startup order is reproducible
- the packaging format is documented and pinned

## Priority 6: Validation and Hardening

Goal:

- validate the state machines after the architecture is fixed

Work items:

- add transition tests for service boundaries
- add revoke / cascade / quarantine tests
- add bootstrap dependency checks
- add audit coverage checks

Done when:

- every state machine has explicit transition coverage
- regressions in bootstrap order or authorization boundaries fail fast

## Recommended Execution Order

1. Freeze the service boundary
2. Define the Microkit service graph
3. Wire seL4 IPC to the service endpoints
4. Finish driver and device lifecycle boundaries
5. Finalize network data plane policy
6. Add image / boot packaging
7. Expand validation
