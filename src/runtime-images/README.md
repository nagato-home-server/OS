# Bundled Runtime Images

Each subdirectory maps to a HubOS runtime profile and version.

Expected shape:

```text
src/runtime-images/<profile>/<version>/
  kernel.elf
  initramfs.cpio.gz
  rootfs.img
  manifest.json
```

Some runtimes omit `initramfs.cpio.gz` or `rootfs.img` when the runtime does
not need them.
