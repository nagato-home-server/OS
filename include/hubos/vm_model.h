#ifndef HUBOS_VM_MODEL_H
#define HUBOS_VM_MODEL_H

#include "hubos/model.h"

typedef struct {
  hubos_id_t id;
  hubos_id_t session_id;
  hubos_id_t guest_memory_id;
  unsigned vcpu_count;
  hubos_id_t virtio_net_session_id;
  hubos_id_t virtio_blk_session_id;
  hubos_id_t vgpu_session_id;
} hubos_vm_t;

static inline void hubos_vm_init(hubos_vm_t *vm,
                                 hubos_id_t id,
                                 hubos_id_t session_id,
                                 hubos_id_t guest_memory_id,
                                 unsigned vcpu_count,
                                 hubos_id_t virtio_net_session_id,
                                 hubos_id_t virtio_blk_session_id,
                                 hubos_id_t vgpu_session_id) {
  if (vm == NULL) {
    return;
  }

  vm->id = id;
  vm->session_id = session_id;
  vm->guest_memory_id = guest_memory_id;
  vm->vcpu_count = vcpu_count;
  vm->virtio_net_session_id = virtio_net_session_id;
  vm->virtio_blk_session_id = virtio_blk_session_id;
  vm->vgpu_session_id = vgpu_session_id;
}

#endif
