# QEMU Workspace Scaffold

This repository now has a bootstrap path that prepares a QEMU-oriented
seL4 workspace on the x86-64 virtual platform.

## What it generates

- a local workspace rooted at `qemu-workspace/` by default
- a symlinked source tree for the local upstream mirror
- a workspace root `CMakeLists.txt` linked to seL4_tools default settings
- a `settings.cmake` that selects `x86_64_generic`
- a `run-qemu.sh` launcher that starts `qemu-system-x86_64` with an explicit
  `kernel` / `initrd` pair
- a `hubos.system` Microkit system description rendered from the selected
  board target
- a `boot-package/` staging area that can attach a built system image

## How to use it

```sh
./scripts/bootstrap-qemu-workspace.sh
./scripts/verify-qemu-scaffold.sh
```

The default board target is `qemu-x86_64_generic`. Override it by setting
`HUBOS_TARGET_BOARD` before running the bootstrap or build scripts.

The launcher expects a kernel image and an initrd image:

```sh
./qemu-workspace/run-qemu.sh /path/to/sel4_32.elf /path/to/loader.img
```

That pair is the seL4 boot path. QEMU should land in the seL4 console first;
Root Task then publishes the service endpoints and drives the VM control-plane
startup path for the default `linux-dev` guest profile.

To build the bootable image from the local upstream mirror and the staged
Microkit SDK, run:

```sh
./scripts/build-qemu-system-image.sh
```

The `boot-package/` directory is the packaging seam. If you set
`HUBOS_BOOT_IMAGE`, it will symlink that image as
`boot-package/system-image.elf` and boot it by default.

The default boot path is therefore:

1. boot seL4 through the x86-64 Microkit `sel4.elf` image plus the QEMU compatibility `sel4_32.elf` copy and `loader.img`
2. reach the seL4 console
3. observe the HubOS control-plane init markers on the seL4 console
4. confirm the HubOS console prints the VM control-plane markers:
   `VM Server: runtime profile=linux-dev`, `VM Server: boot complete`, and
   `Linux VM: control-plane startup confirmed`
5. if you need a guest serial boot transcript, validate the standalone Linux
   image with `./scripts/smoke-linux-vm.sh /tmp/linux-vm.log`

For log capture, set one or both of:

- `QEMU_TRANSCRIPT_FILE` to record the terminal session with `script(1)`
- `QEMU_LOG_FILE` to write QEMU's debug log via `-D`

To smoke-test the current HubOS image under QEMU and verify that the control
plane reaches the expected console markers, run:

```sh
./scripts/smoke-hubos-qemu.sh \
  ./qemu-workspace/build/sel4_32.elf \
  ./qemu-workspace/build/loader.img \
  /tmp/hubos-qemu.log
```

For the standalone Linux guest smoke test, use:

```sh
./scripts/smoke-linux-vm.sh /tmp/linux-vm.log
```

If you only have a build directory, set `HUBOS_BOOT_IMAGE_BUILD_DIR` instead.
The packaging scripts will scan that directory for a Microkit/seL4 image and
attach the first matching artifact they find.

With the CMake target, the resulting launcher lives at:

```sh
build/boot-package/run-qemu.sh
```

The QEMU build helper also stages the x86-64 seL4 kernel image into
`qemu-workspace/build/sel4.elf` and keeps the QEMU compatibility copy at
`qemu-workspace/build/sel4_32.elf` so the local boot package can consume it
directly.

## Current Limit

This now builds a bootable HubOS image from the local source mirror and the
Microkit SDK release tree when the required host toolchain is available.
The generated component stubs are standalone callbacks that are driven by the
upstream Microkit event loop in `libmicrokit`, and the repository keeps a
separate host-side raw protected-call bridge in `src/microkit_kernel_glue.c`
for the model tests. The remaining gap is replacing the repository-rendered
stubs with the actual upstream-generated workspace.
