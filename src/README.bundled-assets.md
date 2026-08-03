# Bundled Assets Under `src/`

This repository keeps the first-stage bundled HubOS assets under `src/` for
now so the image payloads, rollback images, and tool sources can evolve without
introducing a second top-level layout yet.

Current subtrees:

- `src/runtime-images/` - bundled guest runtime kernels, initramfs images, and
  root filesystems
- `src/rollback-images/` - known-good recovery payloads
- `src/tool-src/` - bundled source drops for operator tools such as `nano`
- `src/image-metadata/` - manifests and notes that tie the bundled payloads
  back to HubOS runtime profiles
- `src/runtime-src/` - pinned upstream source trees used to build bundled guest
  runtimes

The files checked in here are placeholders and manifests only. Real payloads
can replace them in-place without changing the higher-level layout.
