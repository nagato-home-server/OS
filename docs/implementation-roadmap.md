# Implementation Roadmap

This repository starts from the Hub-Oriented Session OS specification in `README/spec.md`.

The first goal is not to implement the whole OS at once. The first goal is to pin down the core model and keep every later subsystem aligned with the spec.

## Current Status

Implemented in this repository:

- core resource, capability, session, and DMA model
- resource registry with deduplication by canonical name
- capability manager with issue, mint, transfer, and revoke
- session manager with tree structure and capability cascade revoke
- hub layer with name resolution separated from authorization
- audit log for core lifecycle events
- boot-state validator for ordered startup
- memory manager for frame / hugepage / shared-memory modeling
- bus manager for discovery and registration handoff
- driver registry, trust-only driver loader, and split driver service
- endpoint split for resource, capability, session, hub, driver, and network services
- root-task bootstrap-only control plane
- Microkit IPC request/response model with per-service badge bindings
- host-side runtime stub and endpoint handler wrappers for badge routing
- generated-code shim for the eventual Microkit entrypoint and per-component metadata
- Microkit boot manifest with endpoint/notification/IRQ/shared-memory roles
- memory manager, DMA manager, driver registry, driver loader, and bus managers in the Microkit graph
- network server model with namespace ownership, routing, and NIC selection
- storage and display namespace server models
- Microkit service graph model and boot-order reference
- root-task style system composite tying the managers together
- basic app, container, VM, namespace, and device-server models
- seL4 integration notes, workspace scaffold, and a pinned official GitHub dependency manifest
- source boundary map for native seL4 dependencies vs optional VM backends
- machine-readable source boundary manifest with generated `hubos.xml`
- local upstream mirror staging for offline workspace bootstrap
- build-time network backend switch between lwIP and the optional Linux VM backend
- the live spec/code gap summary in `docs/spec-gap-analysis.md`

## Phase 0: Repository Baseline

Goal:

- Make the repository buildable on a host toolchain.
- Capture the spec as implementation constraints.
- Keep the first code small and testable.

Deliverables:

- `CMakeLists.txt`
- core model headers and implementation
- host-side unit tests

## Phase 1: Core Model

Target subsystems:

- Resource
- Capability
- Session
- DMA mapping state
- Audit event identifiers

Why first:

- Every later subsystem depends on the same vocabulary.
- The spec is stateful, so the state machines must be explicit before drivers or servers are added.

## Phase 2: Trust Base and Registries

Target subsystems:

- Resource Registry
- Capability Manager
- Session Manager
- Memory Manager
- DMA Manager

Key rules:

- Resource IDs are issued only by the Resource Registry.
- Authorization is capability-only.
- DMA is only allowed through the DMA Manager.
- Session creation and revocation must be auditable.

## Phase 3: Hub and Bootstrap

Target subsystems:

- Hub
- Driver Registry
- Driver Loader
- Bus Managers

Key rules:

- The Hub resolves names but does not authorize by name.
- The bootstrap chain must remain acyclic.
- Discovery may propose resources, but the registry decides canonical ownership.

## Phase 4: Device Ownership and Data Plane

Target subsystems:

- Device Servers
- Network Server
- Storage Server
- Display Server
- DMA-backed data paths

Key rules:

- Only Device Servers may own devices.
- The Hub does not forward packets or blocks.
- Direct data movement happens in explicit sessions, not through the control plane.

## Phase 5: Higher-Level Services

Target subsystems:

- Container Server
- VM Server
- App Manager
- Policy enforcement

Key rules:

- Containers and VMs receive delegated capabilities, not raw ownership.
- Inherited policy and namespace views are snapshots, not live links.

## Phase 6: Validation

Target work:

- transition tests for each state machine
- capability delegation tests
- revoke/cascade tests
- bootstrap dependency checks
- audit coverage checks

## First Build Slice

The first build slice should validate only these invariants:

- resource lifecycle transitions
- session lifecycle transitions
- DMA lifecycle transitions
- capability delegation flag handling
- native upstream selection stays separate from the optional Linux VM backend

That is enough to start adding the registry and hub layers without baking in contradictory assumptions.
