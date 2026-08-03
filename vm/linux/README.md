# Linux VM Scaffold

This directory is the placeholder for the Linux guest side of the VM-backed
path. The current flow is x86_64-first and targets `qemu-system-x86_64`.

Keep Linux guest inputs and build helpers here:

- guest kernel sources or pinned build inputs
- initramfs generation
- rootfs staging
- board or guest boot configuration
- scripts for packaging and running the guest image

Suggested subdirectories:

```text
config/
kernel/
initramfs/
rootfs/
scripts/
```

Generated outputs should go under `build/vm/linux/`.

The current scaffold includes:

- `scripts/build-kernel.sh`
- `scripts/build-initramfs.sh`
- `scripts/build-rootfs.sh`
- `scripts/check-libvmm-toolchain.sh`
- `scripts/pack-image.sh`
- `scripts/run-qemu.sh`

The QEMU launcher understands two capture variables:

- `QEMU_TRANSCRIPT_FILE` records the interactive console with `script(1)`
- `QEMU_LOG_FILE` writes QEMU's debug log via `-D`
