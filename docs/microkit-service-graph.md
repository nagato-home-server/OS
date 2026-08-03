# Microkit Service Graph

This repository now carries a host-side model of the Microkit service graph.

## Components

- `Root Task`
- `Resource Registry`
- `Capability Manager`
- `Session Manager`
- `Memory Manager`
- `DMA Manager`
- `Hub`
- `Driver Registry`
- `Driver Loader`
- `Bus Managers`
- `Driver Service`
- `Network Server`
- `Device Server`

## Endpoint Bindings

The host-side Microkit layout currently assigns these endpoint badges:

- `0`: `Root Task`
- `1`: `Resource Registry`
- `2`: `Capability Manager`
- `3`: `Session Manager`
- `4`: `Hub`
- `5`: `Driver Service`
- `6`: `Network Server`
- `12`: `Device Server`

These are the service endpoints that steady-state IPC is wired through.

## Boot Manifest

The boot manifest extends the graph with channel metadata used by generated
Microkit sources:

- `Root Task`: endpoint only, bootstrap-only
- `Resource Registry`: endpoint only
- `Capability Manager`: endpoint only
- `Session Manager`: endpoint only
- `Hub`: endpoint only
- `Memory Manager`: internal only
- `DMA Manager`: notification, shared memory
- `Driver Registry`: internal only
- `Driver Loader`: notification
- `Bus Managers`: notification
- `Driver Service`: endpoint, notification
- `Network Server`: endpoint, notification, shared memory
- `Device Server`: endpoint, notification, IRQ, shared memory

## Properties

- `Root Task` is bootstrap-only
- core and support services are restartable
- memory and DMA managers are part of the restartable trust base
- steady-state communication is endpoint-based
- the graph is acyclic by construction
- network policy owns routing and NIC selection, but not packet payloads

## Control Commands

The graph is meant to be managed by explicit service commands, not by a
single Root Task dispatcher. The important command families are:

- `Root Task`: bootstrap publication and boot-step completion only
- `Session Manager`: tree-preserving operations such as create, ancestry
  checks, child-count queries, state transitions, and cascade revoke
- `Resource Registry` and `Capability Manager`: canonical naming and
  authorization for tree members
- `Driver Service` and `Device Server`: ownership, quarantine, and hardware
  attachment lifecycle
- `Network Server`, `Storage Server`, and `Display Server`: namespace and
  policy ownership
- `VM control`: guest memory, vCPU count, virtio attachment, artifact binding,
  and explicit guest start
- `App`: app-local structure, envelope, session binding, and launch control

The VM command family is required for the final flow where QEMU boots seL4
first, Root Task reaches the console, and Linux is launched later from the
seL4 side.

For multiple apps, the control surface should address `app_<n>` explicitly.
That keeps the app graph inspectable and avoids hard-coding Linux as the only
consumer.

## Boot Order

1. `Root Task`
2. `Resource Registry`
3. `Capability Manager`
4. `Session Manager`
5. `Memory Manager`
6. `DMA Manager`
7. `Hub`
8. `Driver Registry`
9. `Driver Loader`
10. `Bus Managers`
11. `Driver Service`
12. `Network Server`
13. `Device Server`

## Current Use

- validate the service boundary before adding seL4 IPC bindings
- keep the host-side model aligned with the eventual Microkit layout
- use the graph as the reference for boot, restart, and dependency checks
