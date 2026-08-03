# App VM Runtime Selection

HubOS treats App VM guest runtimes as a managed catalog rather than a single
hard-coded OS.

The runtime contract has two parts:

- `runtime_catalog`: the profiles that the system knows how to boot
- `selection`: which profiles are installed, which one is the default, and
  which apps are pinned to a specific runtime

The default config lives at
[`config/hubos-runtime-config.json`](/home/nagatoyuki/Downloads/OS/config/hubos-runtime-config.json).
The first-stage bundled payload layout lives under
[`src/runtime-images/`](/home/nagatoyuki/Downloads/OS/src/runtime-images/README.md).

## Runtime Classes

The current config distinguishes:

- `full`: full guest OS images such as Linux
- `runtime`: smaller app-focused runtimes such as BSD-style service guests or
  unikernel-style images

This keeps "OS-less app payloads" and full guest operating systems behind the
same HubOS control plane.

## Setup Command

Use the setup command for day-to-day changes:

```sh
./scripts/hubos-runtime-setup.sh
```

This interactive mode can:

- choose which runtime profiles are installed
- set the default runtime profile for App VMs
- pin an app to a specific runtime profile
- open the config in `nano` or `$EDITOR` for direct edits

Non-interactive commands are also available:

```sh
./scripts/hubos-runtime-setup.sh show
./scripts/hubos-runtime-setup.sh enable mini-bsd-service
./scripts/hubos-runtime-setup.sh disable unikernel-net
./scripts/hubos-runtime-setup.sh set-default mini-bsd-service
./scripts/hubos-runtime-setup.sh assign telemetry-app unikernel-net
./scripts/hubos-runtime-setup.sh unassign telemetry-app
./scripts/hubos-runtime-setup.sh edit
```

## Config Shape

Each runtime profile should define:

- `id`: HubOS runtime identifier
- `guest_class`: `runtime` or `full`
- `os_family`: `linux`, `bsd`, `unikernel`, or another HubOS-recognized family
- `description`: operator-facing summary
- `version`: bundled base image version
- `update_policy`: who owns post-boot updates
- `source_bundles`: pinned source trees that produce the bundled runtime
- `artifacts`: kernel/rootfs/initramfs/DTB/cmdline inputs
- `artifact_hashes`: sha256 values for path-backed bundled artifacts
- `resources`: default memory, vCPU, and virtio expectations

The `selection` block then records:

- `installed_profiles`
- `default_profile`
- `app_assignments`

## Validation

Use the verifier to catch malformed configs or references to profiles that are
not installed:

```sh
./scripts/verify-runtime-config.sh
```

This validator checks:

- schema version
- unique runtime IDs
- valid default profile
- installed profile references
- app assignment references
- runtime bundle index entries for each profile/version
- runtime manifest fields against the config
- artifact paths pointing into the declared bundle directory
- sha256 values recorded in each runtime bundle manifest
- source bundle names recorded in each runtime bundle manifest

To verify the pinned bundled source checkouts used for runtime construction:

```sh
./scripts/verify-source-bundles.sh
```

To refresh the path-backed artifact list and sha256 values inside bundle
manifests after replacing a bundled runtime payload:

```sh
./scripts/refresh-runtime-bundle-manifest.sh
./scripts/refresh-runtime-bundle-manifest.sh linux-dev
```

## Bundled Sources

For now, bundled runtime payloads and supporting source drops are staged under
`src/`:

- [`src/runtime-images/`](/home/nagatoyuki/Downloads/OS/src/runtime-images/README.md)
- [`src/rollback-images/`](/home/nagatoyuki/Downloads/OS/src/rollback-images/README.md)
- [`src/tool-src/nano/`](/home/nagatoyuki/Downloads/OS/src/tool-src/nano/README.md)
- [`src/runtime-src/`](/home/nagatoyuki/Downloads/OS/src/runtime-src)
