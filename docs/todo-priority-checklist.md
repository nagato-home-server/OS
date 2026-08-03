# TODO Priority Checklist

This checklist is derived from `README/spec.md`, `docs/spec-gap-analysis.md`,
and `docs/implementation-roadmap.md`.

## High Priority

- [x] Validate the generated Microkit runtime surface before bootstrap.
- [ ] Wire the real seL4 / Microkit IPC transport for the exposed services.
  - [x] Materialize the generated Microkit protected-call envelope with
        message-register reads and writes.
  - [x] Add service-specific request and response marshalling for all exposed
        services.
  - [x] Route generated entrypoints through a dedicated kernel-glue shim.
  - [x] Add a raw protected-call receive/dispatch bridge in kernel glue.
- [x] Keep Root Task bootstrap-only in both the model and the runtime surface.
- [x] Keep the generated `qemu-workspace/generated/*/main.c` and
      `microkit-generated/generated/*/main.c` stubs aligned with the host-side
      manifest.
- [x] Add endpoint / badge coverage checks for every exposed service binding.

## Medium Priority

- [ ] Close the driver trust-chain gap with cryptographic signature and hash
      verification.
- [ ] Flesh out hardware-backed Device Server ownership for MMIO, IRQ, reset,
  and lifecycle control.
- [ ] Expand Storage Server and Display Server into full lifecycle control
      planes.
- [ ] Make the DMA Manager path more explicit for IOMMU-backed revoke and
      abort handling.
- [ ] Extend bus discovery into richer hardware enumeration and ownership
      handoff.

## Low Priority

- [ ] Tighten packaging and image generation helpers once the IPC surface is
      stable.
- [ ] Add broader policy and regression coverage after the service graph stops
      changing.
- [ ] Polish docs that only reflect already-implemented behavior.

## Current Focus

1. Finish the real seL4 / Microkit IPC wiring.
2. Make the service graph and generated sources stay in sync.
3. Use the remaining model gaps as regression tests for future changes.
