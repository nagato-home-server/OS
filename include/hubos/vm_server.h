#ifndef HUBOS_VM_SERVER_H
#define HUBOS_VM_SERVER_H

#include <string.h>

#include "hubos/app_vm_runtime.h"
#include "hubos/hub.h"

typedef enum {
  HUBOS_VM_STOPPED = 0,
  HUBOS_VM_BOOTING,
  HUBOS_VM_RUNNING,
  HUBOS_VM_FAILED,
} hubos_vm_state_t;

typedef enum {
  HUBOS_VM_RESTART_MANUAL = 0,
  HUBOS_VM_RESTART_AUTO,
} hubos_vm_restart_policy_t;

typedef struct {
  const char *backend_name;
  hubos_id_t id;
  hubos_id_t owner_session_id;
  hubos_vm_t vm;
  hubos_linux_vm_artifacts_t artifacts;
  const hubos_app_vm_runtime_profile_t *runtime_profile;
  unsigned default_memory_mb;
  hubos_vm_state_t state;
  hubos_vm_restart_policy_t restart_policy;
  unsigned max_restart_attempts;
  unsigned restart_attempts;
  unsigned last_failure_code;
} hubos_vm_server_t;

static inline void hubos_vm_server_init(hubos_vm_server_t *server,
                                        const char *backend_name,
                                        hubos_id_t id,
                                        hubos_id_t owner_session_id,
                                        hubos_vm_t vm,
                                        hubos_linux_vm_artifacts_t artifacts) {
  if (server == NULL) {
    return;
  }

  server->backend_name = backend_name;
  server->id = id;
  server->owner_session_id = owner_session_id;
  server->vm = vm;
  server->artifacts = artifacts;
  server->runtime_profile = NULL;
  server->default_memory_mb = 0;
  server->state = HUBOS_VM_STOPPED;
  server->restart_policy = HUBOS_VM_RESTART_MANUAL;
  server->max_restart_attempts = 0;
  server->restart_attempts = 0;
  server->last_failure_code = 0;
}

static inline bool hubos_vm_server_is_configured(const hubos_vm_server_t *server) {
  return server != NULL &&
         server->vm.guest_memory_id != HUBOS_ID_INVALID &&
         server->vm.vcpu_count > 0 &&
         server->artifacts.kernel_image != NULL;
}

static inline bool hubos_vm_server_attach_guest_memory(hubos_vm_server_t *server,
                                                       hubos_id_t guest_memory_id) {
  if (server == NULL || guest_memory_id == HUBOS_ID_INVALID) {
    return false;
  }

  server->vm.guest_memory_id = guest_memory_id;
  return true;
}

static inline bool hubos_vm_server_set_vcpu_count(hubos_vm_server_t *server, unsigned vcpu_count) {
  if (server == NULL || vcpu_count == 0) {
    return false;
  }

  server->vm.vcpu_count = vcpu_count;
  return true;
}

static inline bool hubos_vm_server_attach_virtio_net_session(hubos_vm_server_t *server,
                                                             hubos_id_t session_id) {
  if (server == NULL || session_id == HUBOS_ID_INVALID) {
    return false;
  }

  server->vm.virtio_net_session_id = session_id;
  return true;
}

static inline bool hubos_vm_server_attach_virtio_blk_session(hubos_vm_server_t *server,
                                                             hubos_id_t session_id) {
  if (server == NULL || session_id == HUBOS_ID_INVALID) {
    return false;
  }

  server->vm.virtio_blk_session_id = session_id;
  return true;
}

static inline bool hubos_vm_server_attach_vgpu_session(hubos_vm_server_t *server,
                                                       hubos_id_t session_id) {
  if (server == NULL || session_id == HUBOS_ID_INVALID) {
    return false;
  }

  server->vm.vgpu_session_id = session_id;
  return true;
}

static inline void hubos_vm_server_set_artifacts(hubos_vm_server_t *server,
                                                 hubos_linux_vm_artifacts_t artifacts) {
  if (server == NULL) {
    return;
  }

  server->artifacts = artifacts;
}

static inline bool hubos_vm_server_select_runtime_profile(
  hubos_vm_server_t *server,
  const hubos_app_vm_runtime_profile_t *profile) {
  if (server == NULL || profile == NULL || profile->id == NULL || profile->artifacts.kernel_image == NULL ||
      profile->resources.vcpus == 0) {
    return false;
  }

  server->runtime_profile = profile;
  server->artifacts = profile->artifacts;
  server->default_memory_mb = profile->resources.memory_mb;
  server->vm.vcpu_count = profile->resources.vcpus;
  if (!profile->resources.virtio_net) {
    server->vm.virtio_net_session_id = HUBOS_ID_INVALID;
  }
  if (!profile->resources.virtio_blk) {
    server->vm.virtio_blk_session_id = HUBOS_ID_INVALID;
  }
  if (!profile->resources.vgpu) {
    server->vm.vgpu_session_id = HUBOS_ID_INVALID;
  }
  return true;
}

static inline bool hubos_vm_server_start(hubos_vm_server_t *server) {
  if (!hubos_vm_server_is_configured(server) || server->state == HUBOS_VM_BOOTING ||
      server->state == HUBOS_VM_RUNNING) {
    return false;
  }

  server->state = HUBOS_VM_BOOTING;
  server->last_failure_code = 0;
  return true;
}

static inline bool hubos_vm_server_complete_boot(hubos_vm_server_t *server) {
  if (server == NULL || server->state != HUBOS_VM_BOOTING) {
    return false;
  }

  server->state = HUBOS_VM_RUNNING;
  server->restart_attempts = 0;
  server->last_failure_code = 0;
  return true;
}

static inline bool hubos_vm_server_set_restart_policy(hubos_vm_server_t *server,
                                                      hubos_vm_restart_policy_t policy,
                                                      unsigned max_restart_attempts) {
  if (server == NULL) {
    return false;
  }

  if (policy == HUBOS_VM_RESTART_MANUAL) {
    max_restart_attempts = 0;
  }

  server->restart_policy = policy;
  server->max_restart_attempts = max_restart_attempts;
  if (policy == HUBOS_VM_RESTART_MANUAL) {
    server->restart_attempts = 0;
  }
  return true;
}

static inline bool hubos_vm_server_fail(hubos_vm_server_t *server,
                                        unsigned failure_code,
                                        bool *out_restarting) {
  if (server == NULL || (server->state != HUBOS_VM_BOOTING && server->state != HUBOS_VM_RUNNING)) {
    return false;
  }

  server->last_failure_code = failure_code;
  if (out_restarting != NULL) {
    *out_restarting = false;
  }
  if (server->restart_policy == HUBOS_VM_RESTART_AUTO &&
      server->restart_attempts < server->max_restart_attempts) {
    server->restart_attempts++;
    server->state = HUBOS_VM_BOOTING;
    if (out_restarting != NULL) {
      *out_restarting = true;
    }
    return true;
  }

  server->state = HUBOS_VM_FAILED;
  return true;
}

static inline bool hubos_vm_server_stop(hubos_vm_server_t *server) {
  if (server == NULL || server->state == HUBOS_VM_STOPPED) {
    return false;
  }

  server->state = HUBOS_VM_STOPPED;
  server->restart_attempts = 0;
  return true;
}

static inline bool hubos_vm_server_uses_virtio_net(const hubos_vm_server_t *server) {
  return server != NULL && server->vm.virtio_net_session_id != HUBOS_ID_INVALID;
}

static inline bool hubos_vm_server_uses_virtio_blk(const hubos_vm_server_t *server) {
  return server != NULL && server->vm.virtio_blk_session_id != HUBOS_ID_INVALID;
}

static inline bool hubos_vm_server_uses_vgpu(const hubos_vm_server_t *server) {
  return server != NULL && server->vm.vgpu_session_id != HUBOS_ID_INVALID;
}

static inline bool hubos_vm_server_matches_runtime_profile(const hubos_vm_server_t *server) {
  if (server == NULL || server->runtime_profile == NULL) {
    return true;
  }

  return server->vm.vcpu_count == server->runtime_profile->resources.vcpus &&
         (!server->runtime_profile->resources.virtio_net || hubos_vm_server_uses_virtio_net(server)) &&
         (!server->runtime_profile->resources.virtio_blk || hubos_vm_server_uses_virtio_blk(server)) &&
         (!server->runtime_profile->resources.vgpu || hubos_vm_server_uses_vgpu(server)) &&
         server->artifacts.kernel_image == server->runtime_profile->artifacts.kernel_image &&
         server->artifacts.initramfs_image == server->runtime_profile->artifacts.initramfs_image &&
         server->artifacts.rootfs_image == server->runtime_profile->artifacts.rootfs_image &&
         server->artifacts.device_tree_blob == server->runtime_profile->artifacts.device_tree_blob &&
         server->artifacts.kernel_cmdline == server->runtime_profile->artifacts.kernel_cmdline;
}

static inline bool hubos_vm_server_describe(const hubos_vm_server_t *server,
                                            hubos_service_descriptor_t *out_descriptor) {
  if (server == NULL || out_descriptor == NULL) {
    return false;
  }

  out_descriptor->resource_id = server->id;
  out_descriptor->name = server->backend_name;
  out_descriptor->name_len = server->backend_name != NULL ? strlen(server->backend_name) : 0;
  out_descriptor->resource_state =
    server->state == HUBOS_VM_RUNNING ? HUBOS_RESOURCE_READY :
    server->state == HUBOS_VM_FAILED ? HUBOS_RESOURCE_FAILED :
    server->state == HUBOS_VM_BOOTING ? HUBOS_RESOURCE_CLASSIFIED :
    hubos_vm_server_is_configured(server) ? HUBOS_RESOURCE_BOUND : HUBOS_RESOURCE_DISCOVERED;
  out_descriptor->endpoint = server->backend_name;
  out_descriptor->version = server->runtime_profile != NULL && server->runtime_profile->id != NULL
                              ? server->runtime_profile->id
                              : server->artifacts.kernel_image;
  out_descriptor->policy_hints =
    (server->vm.guest_memory_id != HUBOS_ID_INVALID ? 1u : 0u) |
    (server->vm.vcpu_count > 0 ? 2u : 0u) |
    (hubos_vm_server_uses_virtio_net(server) ? 4u : 0u) |
    (hubos_vm_server_uses_virtio_blk(server) ? 8u : 0u) |
    (hubos_vm_server_uses_vgpu(server) ? 16u : 0u) |
    (server->artifacts.kernel_image != NULL ? 32u : 0u) |
    (server->artifacts.initramfs_image != NULL ? 64u : 0u) |
    (server->artifacts.rootfs_image != NULL ? 128u : 0u) |
    (server->state == HUBOS_VM_STOPPED ? 256u : 0u) |
    (server->state == HUBOS_VM_BOOTING ? 512u : 0u) |
    (server->state == HUBOS_VM_RUNNING ? 1024u : 0u) |
    (server->state == HUBOS_VM_FAILED ? 2048u : 0u) |
    (server->restart_policy == HUBOS_VM_RESTART_MANUAL ? 4096u : 0u) |
    (server->restart_policy == HUBOS_VM_RESTART_AUTO ? 8192u : 0u) |
    (server->runtime_profile != NULL ? 16384u : 0u) |
    (hubos_vm_server_matches_runtime_profile(server) ? 32768u : 0u);
  return true;
}

#endif
