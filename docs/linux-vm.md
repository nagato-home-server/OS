# Linux VM Layout

This repository treats Linux as a large application running inside a VM
boundary, not as part of the native seL4 control plane.

The preferred VM implementation path is `libvmm`.

The reference asset flow follows the `examples/simple` Linux guest images from
`libvmm`, which provide a prebuilt kernel and initrd/rootfs bundle for a guest
VM. That example is x86_64-oriented, so the fetch path is treated as an
upstream asset source rather than the repository-local build path.

## Why `libvmm`

- it is a seL4 VM monitor library with Linux guest boot support
- it exposes guest image loading for kernel and initramfs
- it already models virtio-based guest devices
- it matches the existing Microkit-first QEMU workflow better than the CAmkES
  example set

Reference upstreams:

- `https://github.com/au-ts/libvmm`
- `https://github.com/sel4/camkes-vm-examples`

`camkes-vm-examples` remains useful as a reference, but the repository should
follow the `libvmm` path for the current Microkit-based integration.

## Suggested Layout

```text
vm/linux/
  README.md
  config/
    kernel-cmdline.txt
    defconfig/
    device-tree/
  kernel/
    <guest kernel source or pinned build inputs>
  initramfs/
    init
    overlay/
  rootfs/
    overlay/
    packages/
  scripts/
    build-kernel.sh
    build-initramfs.sh
    build-rootfs.sh
    pack-image.sh
    run-qemu.sh

build/vm/linux/
  kernel/
    bzImage
  initramfs.cpio.gz
  rootfs.ext4
  linux-vm.bundle
  libvmm-x86_64/
```

## Mapping To The Spec

The spec places Linux under the VM layer:

- `VM Server`
  - `Guest Memory`
  - `vCPU`
  - `virtio-net Session`
  - `virtio-blk Session`
  - `vGPU Session`

That maps to the following implementation roles:

- `Guest Memory` carries the Linux guest RAM image
- `vCPU` carries the Linux execution context
- `virtio-net` connects to the network backend
- `virtio-blk` provides the guest disk / rootfs
- `vGPU` is the graphics path for future hardware-backed display support
- the repo model now carries a dedicated VM server wrapper so these backends can
  be set and described from the control plane rather than as ad hoc fields on
  unrelated servers

The repo model represents this directly with `hubos_vm_t` and the Linux guest
layout wrapper in `include/hubos/linux_vm_layout.h`. The VM control-plane API is
modeled in `include/hubos/vm_server.h`, which gives explicit attach helpers for
guest memory, vCPUs, and the `virtio-*` backends.

## Practical Split

- native seL4 owns policy, ownership, and session brokering
- the VM owns guest execution after Root Task starts it from the seL4 console
- Linux guest user space owns the Linux-side backend logic
- `NetworkManager` lives inside the Linux guest if that backend is selected

The VM control plane should therefore expose explicit commands for guest
memory, vCPU count, virtio attachment, artifact selection, and guest start.
Those commands keep the guest lifecycle visible to the seL4 side instead of
burying it in a standalone launcher.

When the system grows beyond one guest, address apps as `app_<n>` and keep a
separate app structure command so the Hub can place the app before launch.

## Guest Artifacts

Use these artifacts as the concrete inputs:

- guest kernel image
- initramfs image
- rootfs image
- optional DTB or board-specific boot data

The current `run-qemu.sh` scaffold treats `initramfs` as mandatory and `rootfs`
as optional. If a `rootfs` image is present, it is attached as virtio-blk; if it
is omitted, the reference boot flow uses the initramfs-only Buildroot image.

The exact file names can vary, but they should be staged under `build/vm/linux/`
and not mixed into the native `qemu-workspace/` tree.

For the `libvmm` reference bundle, the assets live under:

- `build/vm/linux/libvmm-x86_64/bzImage`
- `build/vm/linux/libvmm-x86_64/rootfs.cpio.gz`

The standalone QEMU smoke test for those assets is:

```sh
timeout 20s qemu-system-x86_64 \
  -machine q35 \
  -m 1024M \
  -nographic \
  -kernel build/vm/linux/libvmm-x86_64/bzImage \
  -initrd build/vm/linux/libvmm-x86_64/rootfs.cpio.gz \
  -append "console=ttyS0,115200 earlyprintk=serial,0x3f8,115200 earlycon=serial,0x3f8,115200 loglevel=8 root=/dev/ram0 rw rdinit=/init"
```

For the staged assets already present in this repository snapshot, the
convenience launcher is `scripts/run-linux-vm.sh`.

To capture a full interactive transcript of that session, set
`QEMU_TRANSCRIPT_FILE`. To collect QEMU's internal debug log as well, set
`QEMU_LOG_FILE`.

To smoke-test the boot transcript after a timed run, use:

```sh
timeout 10s env QEMU_TRANSCRIPT_FILE=/tmp/linux-vm.log ./scripts/run-linux-vm.sh
./scripts/verify-linux-vm-transcript.sh /tmp/linux-vm.log
```

## Toolchain Check

Before trying to build the `libvmm` example locally, confirm the host tools are
present:

- `make`
- `dtc`
- `clang`
- `llvm-objcopy`
- `qemu-system-x86_64`
- `MICROKIT_SDK`

The helper script `vm/linux/scripts/check-libvmm-toolchain.sh` performs that
check and reports missing pieces explicitly.
