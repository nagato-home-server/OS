# Reuse Assets

This document lists upstream assets that are candidates for reuse in HubOS.

## License Policy

HubOS treats GPL-2.0 as the baseline for kernel-adjacent code and prefers
upstream assets that are:

- available from a public `git` repository
- accompanied by a clear license file or SPDX-reported license
- GPL-2.0 compatible
- usable without introducing a proprietary control plane

The practical rule is:

- native seL4 / Microkit path: stay on GPL-2.0-compatible upstreams
- optional Linux VM backend: may use separate upstreams, but keep them behind a
  backend seam so they do not become mandatory for the native path

When a dependency is only used through a separate process or VM boundary, it may
be considered even if it is not the primary license target, but it still needs a
clear integration role and a pinned upstream source.

## Core seL4 Stack

These are the primary candidates for a native seL4 deployment:

- `seL4/seL4.git`
- `seL4/microkit.git`
- `seL4/sel4runtime.git`
- `seL4/seL4_libs.git`
- `seL4/util_libs.git`
- `seL4/musllibc.git`

Why these first:

- they define the kernel boundary, Microkit deployment shape, runtime support,
  and low-level user-space support libraries
- they align with the existing host-side model and generated Microkit stubs

## Native Networking

For a native seL4 network stack, the preferred candidate is:

- `lwip-tcpip/lwip.git`

Why:

- it is a small embedded TCP/IP stack
- it fits a user-space network server model
- it avoids dragging a Linux-only control plane into the native path

## Linux VM Networking Backend

If the network plane is hosted inside a Linux VM, the optional backend candidate is:

- `NetworkManager/NetworkManager`

Why:

- it can manage routes, devices, and connection policy in a Linux guest
- it is suitable for the VM-backed fallback path, not the native seL4 path
- it stays out of the native trust base and is therefore treated as an optional
  backend, not a kernel-adjacent dependency

## Boot and Tooling

These are useful for boot-time integration and generated artifacts:

- `riscv/opensbi.git`
- `nanopb/nanopb.git`
- `seL4/sel4_projects_libs.git`
- `seL4/seL4_tools.git`
- `seL4/sel4test.git`
- `seL4/camkes-tool.git`
- `seL4/sel4bench.git`

## Board Drivers

Target-board-specific driver or BSP repositories should be added through the
`driver_projects` source-boundary list once a hardware target is chosen.

Prefer these when the driver is clearly board-owned rather than part of the
core HubOS control plane:

- device tree parsing helpers
- vendor BSP repositories
- hardware-specific MMIO / IRQ tables
- board driver code that is reused across multiple services

## Notes

- The repository keeps the host-side policy model separate from the upstream
  kernel/runtime stack.
- Candidate assets should be pinned only after confirming they fit the target
  role and license.
- The current scaffold already reflects the native seL4 / Microkit split and
  exposes a backend seam for the Linux VM networking path.
- If a candidate can be pulled directly from `git`, prefer that source over
  vendored snapshots or reimplemented substitutes.
