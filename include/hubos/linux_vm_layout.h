#ifndef HUBOS_LINUX_VM_LAYOUT_H
#define HUBOS_LINUX_VM_LAYOUT_H

#include "hubos/vm_model.h"

typedef struct {
  const char *kernel_image;
  const char *initramfs_image;
  const char *rootfs_image;
  const char *device_tree_blob;
  const char *kernel_cmdline;
} hubos_linux_vm_artifacts_t;

typedef struct {
  const char *backend_name;
  hubos_vm_t vm;
  hubos_linux_vm_artifacts_t artifacts;
} hubos_linux_vm_layout_t;

static inline void hubos_linux_vm_artifacts_init(hubos_linux_vm_artifacts_t *artifacts,
                                                 const char *kernel_image,
                                                 const char *initramfs_image,
                                                 const char *rootfs_image,
                                                 const char *device_tree_blob,
                                                 const char *kernel_cmdline) {
  if (artifacts == NULL) {
    return;
  }

  artifacts->kernel_image = kernel_image;
  artifacts->initramfs_image = initramfs_image;
  artifacts->rootfs_image = rootfs_image;
  artifacts->device_tree_blob = device_tree_blob;
  artifacts->kernel_cmdline = kernel_cmdline;
}

static inline void hubos_linux_vm_layout_init(hubos_linux_vm_layout_t *layout,
                                              const char *backend_name,
                                              hubos_vm_t vm,
                                              hubos_linux_vm_artifacts_t artifacts) {
  if (layout == NULL) {
    return;
  }

  layout->backend_name = backend_name;
  layout->vm = vm;
  layout->artifacts = artifacts;
}

static inline bool hubos_linux_vm_layout_uses_virtio_net(const hubos_linux_vm_layout_t *layout) {
  return layout != NULL && layout->vm.virtio_net_session_id != 0;
}

static inline bool hubos_linux_vm_layout_uses_virtio_blk(const hubos_linux_vm_layout_t *layout) {
  return layout != NULL && layout->vm.virtio_blk_session_id != 0;
}

static inline bool hubos_linux_vm_layout_uses_vgpu(const hubos_linux_vm_layout_t *layout) {
  return layout != NULL && layout->vm.vgpu_session_id != 0;
}

#endif
