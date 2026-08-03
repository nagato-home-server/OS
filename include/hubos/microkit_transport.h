#ifndef HUBOS_MICROKIT_TRANSPORT_H
#define HUBOS_MICROKIT_TRANSPORT_H

#include <microkit.h>

#include "hubos/microkit_ipc.h"

enum {
  HUBOS_MICROKIT_TRANSPORT_LABEL = 0,
  HUBOS_MICROKIT_TRANSPORT_MAX_WORDS = 24,
};

typedef struct {
  seL4_Word words[HUBOS_MICROKIT_TRANSPORT_MAX_WORDS];
  seL4_Uint16 count;
} hubos_microkit_transport_frame_t;

static inline void hubos_microkit_transport_frame_init(hubos_microkit_transport_frame_t *frame) {
  if (frame == NULL) {
    return;
  }

  for (size_t index = 0; index < HUBOS_MICROKIT_TRANSPORT_MAX_WORDS; ++index) {
    frame->words[index] = 0;
  }
  frame->count = 0;
}

static inline bool hubos_microkit_transport_frame_from_msginfo(
  hubos_microkit_transport_frame_t *frame,
  microkit_msginfo msginfo) {
  if (frame == NULL) {
    return false;
  }

  frame->count = (seL4_Uint16)microkit_msginfo_get_count(msginfo);
  if (frame->count > HUBOS_MICROKIT_TRANSPORT_MAX_WORDS) {
    frame->count = 0;
    return false;
  }

  for (seL4_Uint16 index = 0; index < frame->count; ++index) {
    frame->words[index] = microkit_mr_get((seL4_Uint8)index);
  }

  return true;
}

static inline microkit_msginfo hubos_microkit_transport_frame_to_msginfo(
  const hubos_microkit_transport_frame_t *frame,
  seL4_Word label) {
  if (frame == NULL) {
    return microkit_msginfo_new(0, 0);
  }

  return microkit_msginfo_new(label, frame->count);
}

static inline void hubos_microkit_transport_frame_to_mrs(
  const hubos_microkit_transport_frame_t *frame) {
  if (frame == NULL) {
    return;
  }

  for (seL4_Uint16 index = 0; index < frame->count; ++index) {
    microkit_mr_set((seL4_Uint8)index, frame->words[index]);
  }
}

static inline bool hubos_microkit_transport_write_word(hubos_microkit_transport_frame_t *frame,
                                                       size_t index,
                                                       seL4_Word value) {
  if (frame == NULL || index >= HUBOS_MICROKIT_TRANSPORT_MAX_WORDS) {
    return false;
  }

  frame->words[index] = value;
  if (frame->count <= index) {
    frame->count = (seL4_Uint16)(index + 1);
  }
  return true;
}

static inline bool hubos_microkit_transport_read_word(
  const hubos_microkit_transport_frame_t *frame,
  size_t index,
  seL4_Word *out_value) {
  if (frame == NULL || out_value == NULL || index >= frame->count) {
    return false;
  }

  *out_value = frame->words[index];
  return true;
}

static inline bool hubos_microkit_transport_write_bool(hubos_microkit_transport_frame_t *frame,
                                                       size_t index,
                                                       bool value) {
  return hubos_microkit_transport_write_word(frame, index, value ? 1u : 0u);
}

static inline bool hubos_microkit_transport_read_bool(const hubos_microkit_transport_frame_t *frame,
                                                      size_t index,
                                                      bool *out_value) {
  seL4_Word word = 0;

  if (out_value == NULL || !hubos_microkit_transport_read_word(frame, index, &word)) {
    return false;
  }

  *out_value = word != 0;
  return true;
}

static inline bool hubos_microkit_transport_write_u64(hubos_microkit_transport_frame_t *frame,
                                                      size_t index,
                                                      hubos_id_t value) {
  return hubos_microkit_transport_write_word(frame, index, (seL4_Word)value);
}

static inline bool hubos_microkit_transport_read_u64(const hubos_microkit_transport_frame_t *frame,
                                                     size_t index,
                                                     hubos_id_t *out_value) {
  seL4_Word word = 0;

  if (out_value == NULL || !hubos_microkit_transport_read_word(frame, index, &word)) {
    return false;
  }

  *out_value = (hubos_id_t)word;
  return true;
}

static inline bool hubos_microkit_transport_write_ptr(hubos_microkit_transport_frame_t *frame,
                                                      size_t index,
                                                      const void *ptr) {
  return hubos_microkit_transport_write_word(frame, index, (seL4_Word)(uintptr_t)ptr);
}

static inline bool hubos_microkit_transport_read_ptr(const hubos_microkit_transport_frame_t *frame,
                                                     size_t index,
                                                     const void **out_ptr) {
  seL4_Word word = 0;

  if (out_ptr == NULL || !hubos_microkit_transport_read_word(frame, index, &word)) {
    return false;
  }

  *out_ptr = (const void *)(uintptr_t)word;
  return true;
}

static inline size_t hubos_microkit_transport_request_word_count(
  const hubos_microkit_ipc_request_t *request) {
  if (request == NULL) {
    return 0;
  }

  switch (request->service) {
  case HUBOS_MICROKIT_COMPONENT_ROOT_TASK:
    switch ((hubos_root_task_operation_t)request->operation) {
    case HUBOS_ROOT_TASK_OP_BOOTSTRAP:
      return 2;
    case HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP:
    case HUBOS_ROOT_TASK_OP_QUERY_BOOT_STEP:
    case HUBOS_ROOT_TASK_OP_ADVANCE_CONTROL_PLANE:
      return 3;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY:
    switch ((hubos_microkit_resource_operation_t)request->operation) {
    case HUBOS_MICROKIT_RESOURCE_OP_REGISTER:
      return 5;
    case HUBOS_MICROKIT_RESOURCE_OP_UPDATE_STATE:
    case HUBOS_MICROKIT_RESOURCE_OP_QUARANTINE:
    case HUBOS_MICROKIT_RESOURCE_OP_RETIRE:
    case HUBOS_MICROKIT_RESOURCE_OP_DESCRIBE:
      return 3;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER:
    switch ((hubos_microkit_capability_operation_t)request->operation) {
    case HUBOS_MICROKIT_CAPABILITY_OP_ISSUE:
    case HUBOS_MICROKIT_CAPABILITY_OP_MINT_FROM:
      return 6;
    case HUBOS_MICROKIT_CAPABILITY_OP_COPY:
    case HUBOS_MICROKIT_CAPABILITY_OP_TRANSFER:
      return 4;
    case HUBOS_MICROKIT_CAPABILITY_OP_REVOKE:
      return 3;
    case HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE:
      return 5;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER:
    switch ((hubos_microkit_session_operation_t)request->operation) {
    case HUBOS_MICROKIT_SESSION_OP_CREATE:
    case HUBOS_MICROKIT_SESSION_OP_REFRESH_CONTEXT:
      return 5;
    case HUBOS_MICROKIT_SESSION_OP_SET_STATE:
    case HUBOS_MICROKIT_SESSION_OP_IS_ANCESTOR:
      return 4;
    case HUBOS_MICROKIT_SESSION_OP_CHILD_COUNT:
    case HUBOS_MICROKIT_SESSION_OP_REVOKE_TREE:
      return 3;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_HUB:
    switch ((hubos_microkit_hub_operation_t)request->operation) {
    case HUBOS_MICROKIT_HUB_OP_RESOLVE:
      return 4;
    case HUBOS_MICROKIT_HUB_OP_AUTHORIZE:
      return 5;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE:
    switch ((hubos_microkit_driver_operation_t)request->operation) {
    case HUBOS_MICROKIT_DRIVER_OP_BIND:
    case HUBOS_MICROKIT_DRIVER_OP_REBIND:
      return 14;
    case HUBOS_MICROKIT_DRIVER_OP_QUARANTINE:
    case HUBOS_MICROKIT_DRIVER_OP_UNBIND:
      return 3;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER:
    switch ((hubos_microkit_network_operation_t)request->operation) {
    case HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE:
      return 11;
    case HUBOS_MICROKIT_NETWORK_OP_SET_POLICY:
    case HUBOS_MICROKIT_NETWORK_OP_SET_DEFAULT_ROUTE:
      return 4;
    case HUBOS_MICROKIT_NETWORK_OP_ADD_ROUTE:
      return 6;
    case HUBOS_MICROKIT_NETWORK_OP_SELECT_NIC:
      return 4;
    case HUBOS_MICROKIT_NETWORK_OP_BIND_PORT:
      return 5;
    case HUBOS_MICROKIT_NETWORK_OP_SET_FAILOVER_POLICY:
      return 5;
    case HUBOS_MICROKIT_NETWORK_OP_DESCRIBE:
      return 2;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER:
    switch ((hubos_microkit_storage_operation_t)request->operation) {
    case HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE:
      return 11;
    case HUBOS_MICROKIT_STORAGE_OP_RELEASE_NAMESPACE:
    case HUBOS_MICROKIT_STORAGE_OP_FINALIZE_NAMESPACE:
    case HUBOS_MICROKIT_STORAGE_OP_DESCRIBE:
      return 2;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER:
    switch ((hubos_microkit_display_operation_t)request->operation) {
    case HUBOS_MICROKIT_DISPLAY_OP_BIND_NAMESPACE:
      return 11;
    case HUBOS_MICROKIT_DISPLAY_OP_RELEASE_NAMESPACE:
    case HUBOS_MICROKIT_DISPLAY_OP_FINALIZE_NAMESPACE:
    case HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE:
      return 2;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER:
    switch ((hubos_microkit_device_operation_t)request->operation) {
    case HUBOS_MICROKIT_DEVICE_OP_SET_OWNER:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_MMIO:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_IRQ:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_DMA:
      return 3;
    case HUBOS_MICROKIT_DEVICE_OP_RELEASE_OWNER:
    case HUBOS_MICROKIT_DEVICE_OP_RESET:
    case HUBOS_MICROKIT_DEVICE_OP_QUARANTINE:
    case HUBOS_MICROKIT_DEVICE_OP_CLEAR_QUARANTINE:
    case HUBOS_MICROKIT_DEVICE_OP_DESCRIBE:
      return 2;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_VM_SERVER:
    switch ((hubos_microkit_vm_operation_t)request->operation) {
    case HUBOS_MICROKIT_VM_OP_SET_GUEST_MEMORY:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_NET:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_BLK:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VGPU:
      return 3;
    case HUBOS_MICROKIT_VM_OP_SET_VCPU_COUNT:
      return 3;
    case HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE:
      return 4;
    case HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS:
      return 7;
    case HUBOS_MICROKIT_VM_OP_SET_RESTART_POLICY:
      return 4;
    case HUBOS_MICROKIT_VM_OP_START:
    case HUBOS_MICROKIT_VM_OP_COMPLETE_BOOT:
    case HUBOS_MICROKIT_VM_OP_STOP:
    case HUBOS_MICROKIT_VM_OP_DESCRIBE:
      return 2;
    case HUBOS_MICROKIT_VM_OP_FAIL:
      return 3;
    }
    break;
  default:
    break;
  }

  return 0;
}

static inline size_t hubos_microkit_transport_response_word_count(
  const hubos_microkit_ipc_response_t *response) {
  if (response == NULL) {
    return 0;
  }

  return 16;
}

static inline void hubos_microkit_transport_request_init(
  hubos_microkit_ipc_request_t *request,
  hubos_microkit_component_kind_t service,
  unsigned operation) {
  if (request == NULL) {
    return;
  }

  *request = (hubos_microkit_ipc_request_t){0};
  request->service = service;
  request->operation = operation;
}

static inline bool hubos_microkit_transport_encode_driver_package(
  hubos_microkit_transport_frame_t *frame,
  size_t *offset,
  const hubos_driver_package_t *package) {
  if (frame == NULL || offset == NULL || package == NULL) {
    return false;
  }

  if (*offset + 10 > HUBOS_MICROKIT_TRANSPORT_MAX_WORDS) {
    return false;
  }

  hubos_microkit_transport_write_ptr(frame, (*offset)++, package->manifest);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, package->binary);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, package->signature);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, package->hash);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, package->version);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, package->signing_key_id);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, package->dependencies);
  hubos_microkit_transport_write_word(frame, (*offset)++, (seL4_Word)package->dependency_count);
  hubos_microkit_transport_write_word(frame, (*offset)++, package->platform_abi_version);
  hubos_microkit_transport_write_word(frame, (*offset)++, package->minimum_platform_abi_version);
  return true;
}

static inline bool hubos_microkit_transport_decode_driver_package(
  const hubos_microkit_transport_frame_t *frame,
  size_t *offset,
  hubos_driver_package_t *package) {
  const void *ptr = NULL;
  seL4_Word word = 0;

  if (frame == NULL || offset == NULL || package == NULL) {
    return false;
  }

  if (*offset + 10 > frame->count) {
    return false;
  }

  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  package->manifest = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  package->binary = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  package->signature = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  package->hash = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  package->version = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  package->signing_key_id = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  package->dependencies = (const char *const *)ptr;
  hubos_microkit_transport_read_word(frame, (*offset)++, &word);
  package->dependency_count = (size_t)word;
  hubos_microkit_transport_read_word(frame, (*offset)++, &word);
  package->platform_abi_version = (unsigned)word;
  hubos_microkit_transport_read_word(frame, (*offset)++, &word);
  package->minimum_platform_abi_version = (unsigned)word;
  return true;
}

static inline bool hubos_microkit_transport_encode_namespace_handle(
  hubos_microkit_transport_frame_t *frame,
  size_t *offset,
  const hubos_namespace_handle_t *handle) {
  if (frame == NULL || offset == NULL || handle == NULL) {
    return false;
  }

  if (*offset + 9 > HUBOS_MICROKIT_TRANSPORT_MAX_WORDS) {
    return false;
  }

  hubos_microkit_transport_write_u64(frame, (*offset)++, handle->id);
  hubos_microkit_transport_write_word(frame, (*offset)++, (seL4_Word)handle->kind);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, handle->name);
  hubos_microkit_transport_write_bool(frame, (*offset)++, handle->owned_by_server);
  hubos_microkit_transport_write_u64(frame, (*offset)++, handle->lifecycle.id);
  hubos_microkit_transport_write_u64(frame, (*offset)++, handle->lifecycle.owner_session_id);
  hubos_microkit_transport_write_word(frame, (*offset)++, (seL4_Word)handle->lifecycle.refcount);
  hubos_microkit_transport_write_word(frame, (*offset)++, (seL4_Word)handle->lifecycle.state);
  hubos_microkit_transport_write_bool(frame, (*offset)++, handle->lifecycle.pending_finalization);
  return true;
}

static inline bool hubos_microkit_transport_decode_namespace_handle(
  const hubos_microkit_transport_frame_t *frame,
  size_t *offset,
  hubos_namespace_handle_t *handle) {
  const void *ptr = NULL;
  seL4_Word word = 0;

  if (frame == NULL || offset == NULL || handle == NULL) {
    return false;
  }

  if (*offset + 9 > frame->count) {
    return false;
  }

  hubos_microkit_transport_read_u64(frame, (*offset)++, &handle->id);
  hubos_microkit_transport_read_word(frame, (*offset)++, &word);
  handle->kind = (hubos_namespace_kind_t)word;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  handle->name = (const char *)ptr;
  hubos_microkit_transport_read_bool(frame, (*offset)++, &handle->owned_by_server);
  hubos_microkit_transport_read_u64(frame, (*offset)++, &handle->lifecycle.id);
  hubos_microkit_transport_read_u64(frame, (*offset)++, &handle->lifecycle.owner_session_id);
  hubos_microkit_transport_read_word(frame, (*offset)++, &word);
  handle->lifecycle.refcount = (size_t)word;
  hubos_microkit_transport_read_word(frame, (*offset)++, &word);
  handle->lifecycle.state = (hubos_shared_resource_state_t)word;
  hubos_microkit_transport_read_bool(frame, (*offset)++, &handle->lifecycle.pending_finalization);
  return true;
}

static inline bool hubos_microkit_transport_encode_vm_artifacts(
  hubos_microkit_transport_frame_t *frame,
  size_t *offset,
  const hubos_linux_vm_artifacts_t *artifacts) {
  if (frame == NULL || offset == NULL || artifacts == NULL) {
    return false;
  }

  if (*offset + 5 > HUBOS_MICROKIT_TRANSPORT_MAX_WORDS) {
    return false;
  }

  hubos_microkit_transport_write_ptr(frame, (*offset)++, artifacts->kernel_image);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, artifacts->initramfs_image);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, artifacts->rootfs_image);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, artifacts->device_tree_blob);
  hubos_microkit_transport_write_ptr(frame, (*offset)++, artifacts->kernel_cmdline);
  return true;
}

static inline bool hubos_microkit_transport_decode_vm_artifacts(
  const hubos_microkit_transport_frame_t *frame,
  size_t *offset,
  hubos_linux_vm_artifacts_t *artifacts) {
  const void *ptr = NULL;

  if (frame == NULL || offset == NULL || artifacts == NULL) {
    return false;
  }

  if (*offset + 5 > frame->count) {
    return false;
  }

  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  artifacts->kernel_image = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  artifacts->initramfs_image = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  artifacts->rootfs_image = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  artifacts->device_tree_blob = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, (*offset)++, &ptr);
  artifacts->kernel_cmdline = (const char *)ptr;
  return true;
}

static inline bool hubos_microkit_transport_request_encode(
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_transport_frame_t *frame) {
  size_t offset = 0;

  if (request == NULL || frame == NULL) {
    return false;
  }

  hubos_microkit_transport_frame_init(frame);
  if (!hubos_microkit_transport_write_word(frame, offset++, (seL4_Word)request->service) ||
      !hubos_microkit_transport_write_word(frame, offset++, (seL4_Word)request->operation)) {
    return false;
  }

  switch (request->service) {
  case HUBOS_MICROKIT_COMPONENT_ROOT_TASK:
    switch ((hubos_root_task_operation_t)request->operation) {
    case HUBOS_ROOT_TASK_OP_BOOTSTRAP:
      break;
    case HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP:
    case HUBOS_ROOT_TASK_OP_QUERY_BOOT_STEP:
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          (seL4_Word)request->payload.boot_step.step);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY:
    switch ((hubos_microkit_resource_operation_t)request->operation) {
    case HUBOS_MICROKIT_RESOURCE_OP_REGISTER:
      hubos_microkit_transport_write_ptr(frame, offset++, request->payload.resource_register.name);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          (seL4_Word)request->payload.resource_register.name_len);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          (seL4_Word)request->payload.resource_register.state);
      break;
    case HUBOS_MICROKIT_RESOURCE_OP_UPDATE_STATE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.resource_update_state.resource_id);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          (seL4_Word)request->payload.resource_update_state.state);
      break;
    case HUBOS_MICROKIT_RESOURCE_OP_QUARANTINE:
    case HUBOS_MICROKIT_RESOURCE_OP_RETIRE:
    case HUBOS_MICROKIT_RESOURCE_OP_DESCRIBE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.resource_update_state.resource_id);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER:
    switch ((hubos_microkit_capability_operation_t)request->operation) {
    case HUBOS_MICROKIT_CAPABILITY_OP_ISSUE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_issue.owner_session_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_issue.resource_id);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.capability_issue.rights);
      hubos_microkit_transport_write_bool(frame,
                                         offset++,
                                         request->payload.capability_issue.delegatable);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_COPY:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_copy.source_capability_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_copy.owner_session_id);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_MINT_FROM:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_mint.source_capability_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_mint.owner_session_id);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.capability_mint.rights);
      hubos_microkit_transport_write_bool(frame,
                                         offset++,
                                         request->payload.capability_mint.delegatable);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_TRANSFER:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_transfer.capability_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_transfer.new_owner_session_id);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_REVOKE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_revoke.capability_id);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_authorize.capability_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.capability_authorize.resource_id);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.capability_authorize.required_rights);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER:
    switch ((hubos_microkit_session_operation_t)request->operation) {
    case HUBOS_MICROKIT_SESSION_OP_CREATE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_create.owner_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_create.parent_id);
      hubos_microkit_transport_write_word(frame, offset++, request->payload.session_create.type);
      break;
    case HUBOS_MICROKIT_SESSION_OP_REFRESH_CONTEXT:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_refresh_context.session_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_refresh_context.namespace_view_version);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_refresh_context.policy_context_version);
      break;
    case HUBOS_MICROKIT_SESSION_OP_SET_STATE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_set_state.session_id);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.session_set_state.state);
      break;
    case HUBOS_MICROKIT_SESSION_OP_IS_ANCESTOR:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_is_ancestor.ancestor_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_is_ancestor.session_id);
      break;
    case HUBOS_MICROKIT_SESSION_OP_CHILD_COUNT:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_child_count.session_id);
      break;
    case HUBOS_MICROKIT_SESSION_OP_REVOKE_TREE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.session_revoke_tree.session_id);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_HUB:
    switch ((hubos_microkit_hub_operation_t)request->operation) {
    case HUBOS_MICROKIT_HUB_OP_RESOLVE:
      hubos_microkit_transport_write_ptr(frame, offset++, request->payload.hub_resolve.name);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          (seL4_Word)request->payload.hub_resolve.name_len);
      break;
    case HUBOS_MICROKIT_HUB_OP_AUTHORIZE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.hub_authorize.capability_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.hub_authorize.resource_id);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.hub_authorize.required_rights);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE:
    switch ((hubos_microkit_driver_operation_t)request->operation) {
    case HUBOS_MICROKIT_DRIVER_OP_BIND:
    case HUBOS_MICROKIT_DRIVER_OP_REBIND:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.driver_bind.resource_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.driver_bind.driver_id);
      if (!hubos_microkit_transport_encode_driver_package(frame, &offset, &request->payload.driver_bind.package)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_DRIVER_OP_QUARANTINE:
    case HUBOS_MICROKIT_DRIVER_OP_UNBIND:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.driver_quarantine.resource_id);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER:
    switch ((hubos_microkit_network_operation_t)request->operation) {
    case HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE:
      if (!hubos_microkit_transport_encode_namespace_handle(frame,
                                                             &offset,
                                                             &request->payload.network_bind_namespace.namespace_handle)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_NETWORK_OP_SET_POLICY:
      hubos_microkit_transport_write_bool(frame,
                                         offset++,
                                         request->payload.network_set_policy.routing_enabled);
      hubos_microkit_transport_write_bool(frame,
                                         offset++,
                                         request->payload.network_set_policy.firewall_enabled);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_ADD_ROUTE:
      hubos_microkit_transport_write_ptr(frame,
                                         offset++,
                                         request->payload.network_add_route.destination);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          (seL4_Word)request->payload.network_add_route.destination_len);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.network_add_route.nic_resource_id);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.network_add_route.metric);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_SET_DEFAULT_ROUTE:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.network_set_default_route.nic_resource_id);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_SELECT_NIC:
      hubos_microkit_transport_write_ptr(frame,
                                         offset++,
                                         request->payload.network_select_nic.destination);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          (seL4_Word)request->payload.network_select_nic.destination_len);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_BIND_PORT:
      hubos_microkit_transport_write_word(frame, offset++, request->payload.network_bind_port.port);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.network_bind_port.nic_resource_id);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.network_bind_port.session_id);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_SET_FAILOVER_POLICY:
      hubos_microkit_transport_write_bool(frame,
                                         offset++,
                                         request->payload.network_set_failover_policy.failover_enabled);
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.network_set_failover_policy.preferred_nic_resource_id);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_DESCRIBE:
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER:
    switch ((hubos_microkit_storage_operation_t)request->operation) {
    case HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE:
      if (!hubos_microkit_transport_encode_namespace_handle(frame,
                                                             &offset,
                                                             &request->payload.storage_bind_namespace.namespace_handle)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_STORAGE_OP_RELEASE_NAMESPACE:
    case HUBOS_MICROKIT_STORAGE_OP_FINALIZE_NAMESPACE:
    case HUBOS_MICROKIT_STORAGE_OP_DESCRIBE:
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER:
    switch ((hubos_microkit_display_operation_t)request->operation) {
    case HUBOS_MICROKIT_DISPLAY_OP_BIND_NAMESPACE:
      if (!hubos_microkit_transport_encode_namespace_handle(frame,
                                                             &offset,
                                                             &request->payload.display_bind_namespace.namespace_handle)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_DISPLAY_OP_RELEASE_NAMESPACE:
    case HUBOS_MICROKIT_DISPLAY_OP_FINALIZE_NAMESPACE:
    case HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE:
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER:
    switch ((hubos_microkit_device_operation_t)request->operation) {
    case HUBOS_MICROKIT_DEVICE_OP_SET_OWNER:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_MMIO:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_IRQ:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_DMA:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.device_owner.owner_session_id);
      break;
    case HUBOS_MICROKIT_DEVICE_OP_RELEASE_OWNER:
    case HUBOS_MICROKIT_DEVICE_OP_RESET:
    case HUBOS_MICROKIT_DEVICE_OP_QUARANTINE:
    case HUBOS_MICROKIT_DEVICE_OP_CLEAR_QUARANTINE:
    case HUBOS_MICROKIT_DEVICE_OP_DESCRIBE:
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_VM_SERVER:
    switch ((hubos_microkit_vm_operation_t)request->operation) {
    case HUBOS_MICROKIT_VM_OP_SET_GUEST_MEMORY:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.vm_guest_memory.guest_memory_id);
      break;
    case HUBOS_MICROKIT_VM_OP_SET_VCPU_COUNT:
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.vm_vcpu_count.vcpu_count);
      break;
    case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_NET:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_BLK:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VGPU:
      hubos_microkit_transport_write_u64(frame,
                                         offset++,
                                         request->payload.vm_session.session_id);
      break;
    case HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE:
      hubos_microkit_transport_write_ptr(
        frame, offset++, request->payload.vm_select_runtime_profile.runtime_profile_id);
      hubos_microkit_transport_write_word(
        frame, offset++, (seL4_Word)request->payload.vm_select_runtime_profile.runtime_profile_id_len);
      break;
    case HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS:
      if (!hubos_microkit_transport_encode_vm_artifacts(frame,
                                                        &offset,
                                                        &request->payload.vm_set_artifacts.artifacts)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_VM_OP_SET_RESTART_POLICY:
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.vm_set_restart_policy.policy);
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.vm_set_restart_policy.max_restart_attempts);
      break;
    case HUBOS_MICROKIT_VM_OP_START:
    case HUBOS_MICROKIT_VM_OP_COMPLETE_BOOT:
    case HUBOS_MICROKIT_VM_OP_STOP:
    case HUBOS_MICROKIT_VM_OP_DESCRIBE:
      break;
    case HUBOS_MICROKIT_VM_OP_FAIL:
      hubos_microkit_transport_write_word(frame,
                                          offset++,
                                          request->payload.vm_fail.failure_code);
      break;
    default:
      return false;
    }
    break;
  default:
    return false;
  }

  frame->count = (seL4_Uint16)offset;
  return true;
}

static inline bool hubos_microkit_transport_request_decode(
  const hubos_microkit_transport_frame_t *frame,
  hubos_microkit_ipc_request_t *request) {
  size_t offset = 0;
  seL4_Word word = 0;

  if (frame == NULL || request == NULL || frame->count < 2) {
    return false;
  }

  *request = (hubos_microkit_ipc_request_t){0};

  if (!hubos_microkit_transport_read_word(frame, offset++, &word)) {
    return false;
  }
  request->service = (hubos_microkit_component_kind_t)word;

  if (!hubos_microkit_transport_read_word(frame, offset++, &word)) {
    return false;
  }
  request->operation = (unsigned)word;

  switch (request->service) {
  case HUBOS_MICROKIT_COMPONENT_ROOT_TASK:
    switch ((hubos_root_task_operation_t)request->operation) {
    case HUBOS_ROOT_TASK_OP_BOOTSTRAP:
      break;
    case HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP:
    case HUBOS_ROOT_TASK_OP_QUERY_BOOT_STEP:
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.boot_step.step = (hubos_boot_step_t)word;
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY:
    switch ((hubos_microkit_resource_operation_t)request->operation) {
    case HUBOS_MICROKIT_RESOURCE_OP_REGISTER:
      hubos_microkit_transport_read_ptr(frame, offset++, (const void **)&request->payload.resource_register.name);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.resource_register.name_len = (size_t)word;
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.resource_register.state = (hubos_resource_state_t)word;
      break;
    case HUBOS_MICROKIT_RESOURCE_OP_UPDATE_STATE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.resource_update_state.resource_id);
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.resource_update_state.state = (hubos_resource_state_t)word;
      break;
    case HUBOS_MICROKIT_RESOURCE_OP_QUARANTINE:
    case HUBOS_MICROKIT_RESOURCE_OP_RETIRE:
    case HUBOS_MICROKIT_RESOURCE_OP_DESCRIBE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.resource_update_state.resource_id);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER:
    switch ((hubos_microkit_capability_operation_t)request->operation) {
    case HUBOS_MICROKIT_CAPABILITY_OP_ISSUE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_issue.owner_session_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_issue.resource_id);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.capability_issue.rights = (unsigned)word;
      hubos_microkit_transport_read_bool(frame,
                                         offset++,
                                         &request->payload.capability_issue.delegatable);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_COPY:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_copy.source_capability_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_copy.owner_session_id);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_MINT_FROM:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_mint.source_capability_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_mint.owner_session_id);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.capability_mint.rights = (unsigned)word;
      hubos_microkit_transport_read_bool(frame,
                                         offset++,
                                         &request->payload.capability_mint.delegatable);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_TRANSFER:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_transfer.capability_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_transfer.new_owner_session_id);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_REVOKE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_revoke.capability_id);
      break;
    case HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_authorize.capability_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.capability_authorize.resource_id);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.capability_authorize.required_rights = (unsigned)word;
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER:
    switch ((hubos_microkit_session_operation_t)request->operation) {
    case HUBOS_MICROKIT_SESSION_OP_CREATE:
      hubos_microkit_transport_read_u64(frame, offset++, &request->payload.session_create.owner_id);
      hubos_microkit_transport_read_u64(frame, offset++, &request->payload.session_create.parent_id);
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.session_create.type = (hubos_session_type_t)word;
      break;
    case HUBOS_MICROKIT_SESSION_OP_REFRESH_CONTEXT:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.session_refresh_context.session_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.session_refresh_context.namespace_view_version);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.session_refresh_context.policy_context_version);
      break;
    case HUBOS_MICROKIT_SESSION_OP_SET_STATE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.session_set_state.session_id);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.session_set_state.state = (hubos_session_state_t)word;
      break;
    case HUBOS_MICROKIT_SESSION_OP_IS_ANCESTOR:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.session_is_ancestor.ancestor_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.session_is_ancestor.session_id);
      break;
    case HUBOS_MICROKIT_SESSION_OP_CHILD_COUNT:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.session_child_count.session_id);
      break;
    case HUBOS_MICROKIT_SESSION_OP_REVOKE_TREE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.session_revoke_tree.session_id);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_HUB:
    switch ((hubos_microkit_hub_operation_t)request->operation) {
    case HUBOS_MICROKIT_HUB_OP_RESOLVE:
      hubos_microkit_transport_read_ptr(frame, offset++, (const void **)&request->payload.hub_resolve.name);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.hub_resolve.name_len = (size_t)word;
      break;
    case HUBOS_MICROKIT_HUB_OP_AUTHORIZE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.hub_authorize.capability_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.hub_authorize.resource_id);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.hub_authorize.required_rights = (unsigned)word;
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE:
    switch ((hubos_microkit_driver_operation_t)request->operation) {
    case HUBOS_MICROKIT_DRIVER_OP_BIND:
    case HUBOS_MICROKIT_DRIVER_OP_REBIND:
      hubos_microkit_transport_read_u64(frame, offset++, &request->payload.driver_bind.resource_id);
      hubos_microkit_transport_read_u64(frame, offset++, &request->payload.driver_bind.driver_id);
      if (!hubos_microkit_transport_decode_driver_package(frame, &offset, &request->payload.driver_bind.package)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_DRIVER_OP_QUARANTINE:
    case HUBOS_MICROKIT_DRIVER_OP_UNBIND:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.driver_quarantine.resource_id);
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER:
    switch ((hubos_microkit_network_operation_t)request->operation) {
    case HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE:
      if (!hubos_microkit_transport_decode_namespace_handle(frame,
                                                             &offset,
                                                             &request->payload.network_bind_namespace.namespace_handle)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_NETWORK_OP_SET_POLICY:
      hubos_microkit_transport_read_bool(frame,
                                         offset++,
                                         &request->payload.network_set_policy.routing_enabled);
      hubos_microkit_transport_read_bool(frame,
                                         offset++,
                                         &request->payload.network_set_policy.firewall_enabled);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_ADD_ROUTE:
      hubos_microkit_transport_read_ptr(frame,
                                        offset++,
                                        (const void **)&request->payload.network_add_route.destination);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.network_add_route.destination_len = (size_t)word;
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.network_add_route.nic_resource_id);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.network_add_route.metric = (unsigned)word;
      break;
    case HUBOS_MICROKIT_NETWORK_OP_SET_DEFAULT_ROUTE:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.network_set_default_route.nic_resource_id);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_SELECT_NIC:
      hubos_microkit_transport_read_ptr(frame,
                                        offset++,
                                        (const void **)&request->payload.network_select_nic.destination);
      hubos_microkit_transport_read_word(frame,
                                         offset++,
                                         &word);
      request->payload.network_select_nic.destination_len = (size_t)word;
      break;
    case HUBOS_MICROKIT_NETWORK_OP_BIND_PORT:
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.network_bind_port.port = (unsigned)word;
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.network_bind_port.nic_resource_id);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.network_bind_port.session_id);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_SET_FAILOVER_POLICY:
      hubos_microkit_transport_read_bool(frame,
                                         offset++,
                                         &request->payload.network_set_failover_policy.failover_enabled);
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.network_set_failover_policy.preferred_nic_resource_id);
      break;
    case HUBOS_MICROKIT_NETWORK_OP_DESCRIBE:
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER:
    switch ((hubos_microkit_storage_operation_t)request->operation) {
    case HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE:
      if (!hubos_microkit_transport_decode_namespace_handle(frame,
                                                             &offset,
                                                             &request->payload.storage_bind_namespace.namespace_handle)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_STORAGE_OP_RELEASE_NAMESPACE:
    case HUBOS_MICROKIT_STORAGE_OP_FINALIZE_NAMESPACE:
    case HUBOS_MICROKIT_STORAGE_OP_DESCRIBE:
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER:
    switch ((hubos_microkit_display_operation_t)request->operation) {
    case HUBOS_MICROKIT_DISPLAY_OP_BIND_NAMESPACE:
      if (!hubos_microkit_transport_decode_namespace_handle(frame,
                                                             &offset,
                                                             &request->payload.display_bind_namespace.namespace_handle)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_DISPLAY_OP_RELEASE_NAMESPACE:
    case HUBOS_MICROKIT_DISPLAY_OP_FINALIZE_NAMESPACE:
    case HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE:
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER:
    switch ((hubos_microkit_device_operation_t)request->operation) {
    case HUBOS_MICROKIT_DEVICE_OP_SET_OWNER:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_MMIO:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_IRQ:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_DMA:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.device_owner.owner_session_id);
      break;
    case HUBOS_MICROKIT_DEVICE_OP_RELEASE_OWNER:
    case HUBOS_MICROKIT_DEVICE_OP_RESET:
    case HUBOS_MICROKIT_DEVICE_OP_QUARANTINE:
    case HUBOS_MICROKIT_DEVICE_OP_CLEAR_QUARANTINE:
    case HUBOS_MICROKIT_DEVICE_OP_DESCRIBE:
      break;
    default:
      return false;
    }
    break;
  case HUBOS_MICROKIT_COMPONENT_VM_SERVER:
    switch ((hubos_microkit_vm_operation_t)request->operation) {
    case HUBOS_MICROKIT_VM_OP_SET_GUEST_MEMORY:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.vm_guest_memory.guest_memory_id);
      break;
    case HUBOS_MICROKIT_VM_OP_SET_VCPU_COUNT:
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.vm_vcpu_count.vcpu_count = (unsigned)word;
      break;
    case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_NET:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_BLK:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VGPU:
      hubos_microkit_transport_read_u64(frame,
                                        offset++,
                                        &request->payload.vm_session.session_id);
      break;
    case HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE:
      hubos_microkit_transport_read_ptr(frame,
                                        offset++,
                                        (const void **)&request->payload.vm_select_runtime_profile.runtime_profile_id);
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.vm_select_runtime_profile.runtime_profile_id_len = (size_t)word;
      break;
    case HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS:
      if (!hubos_microkit_transport_decode_vm_artifacts(frame,
                                                        &offset,
                                                        &request->payload.vm_set_artifacts.artifacts)) {
        return false;
      }
      break;
    case HUBOS_MICROKIT_VM_OP_SET_RESTART_POLICY:
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.vm_set_restart_policy.policy = (hubos_vm_restart_policy_t)word;
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.vm_set_restart_policy.max_restart_attempts = (unsigned)word;
      break;
    case HUBOS_MICROKIT_VM_OP_START:
    case HUBOS_MICROKIT_VM_OP_COMPLETE_BOOT:
    case HUBOS_MICROKIT_VM_OP_STOP:
    case HUBOS_MICROKIT_VM_OP_DESCRIBE:
      break;
    case HUBOS_MICROKIT_VM_OP_FAIL:
      hubos_microkit_transport_read_word(frame, offset++, &word);
      request->payload.vm_fail.failure_code = (unsigned)word;
      break;
    default:
      return false;
    }
    break;
  default:
    return false;
  }

  return true;
}

static inline bool hubos_microkit_transport_response_encode(
  const hubos_microkit_ipc_response_t *response,
  hubos_microkit_transport_frame_t *frame) {
  size_t offset = 0;

  if (response == NULL || frame == NULL) {
    return false;
  }

  hubos_microkit_transport_frame_init(frame);
  hubos_microkit_transport_write_word(frame, offset++, (seL4_Word)response->status);
  hubos_microkit_transport_write_u64(frame, offset++, response->resource_id);
  hubos_microkit_transport_write_u64(frame, offset++, response->capability_id);
  hubos_microkit_transport_write_u64(frame, offset++, response->session_id);
  hubos_microkit_transport_write_u64(frame, offset++, response->driver_id);
  hubos_microkit_transport_write_word(frame, offset++, (seL4_Word)response->count);
  hubos_microkit_transport_write_bool(frame, offset++, response->is_new);
  hubos_microkit_transport_write_bool(frame, offset++, response->bool_result);
  hubos_microkit_transport_write_word(frame, offset++, (seL4_Word)response->boot_step);
  hubos_microkit_transport_write_u64(frame, offset++, response->descriptor.resource_id);
  hubos_microkit_transport_write_ptr(frame, offset++, response->descriptor.name);
  hubos_microkit_transport_write_word(frame, offset++, (seL4_Word)response->descriptor.name_len);
  hubos_microkit_transport_write_word(frame, offset++, (seL4_Word)response->descriptor.resource_state);
  hubos_microkit_transport_write_ptr(frame, offset++, response->descriptor.endpoint);
  hubos_microkit_transport_write_ptr(frame, offset++, response->descriptor.version);
  hubos_microkit_transport_write_word(frame, offset++, response->descriptor.policy_hints);
  frame->count = (seL4_Uint16)offset;
  return true;
}

static inline bool hubos_microkit_transport_response_decode(
  const hubos_microkit_transport_frame_t *frame,
  hubos_microkit_ipc_response_t *response) {
  size_t offset = 0;
  const void *ptr = NULL;
  seL4_Word word = 0;

  if (frame == NULL || response == NULL || frame->count < 16) {
    return false;
  }

  *response = (hubos_microkit_ipc_response_t){0};
  response->status = HUBOS_IPC_STATUS_INVALID_ARGUMENT;
  response->resource_id = HUBOS_ID_INVALID;
  response->capability_id = HUBOS_ID_INVALID;
  response->session_id = HUBOS_ID_INVALID;
  response->driver_id = HUBOS_ID_INVALID;
  response->boot_step = HUBOS_BOOT_STEP_COUNT;
  response->descriptor.resource_id = HUBOS_ID_INVALID;
  response->descriptor.name = NULL;
  response->descriptor.name_len = 0;
  response->descriptor.resource_state = HUBOS_RESOURCE_DISCOVERED;
  response->descriptor.endpoint = NULL;
  response->descriptor.version = NULL;
  response->descriptor.policy_hints = 0;

  hubos_microkit_transport_read_word(frame, offset++, &word);
  response->status = (hubos_ipc_status_t)word;
  hubos_microkit_transport_read_u64(frame, offset++, &response->resource_id);
  hubos_microkit_transport_read_u64(frame, offset++, &response->capability_id);
  hubos_microkit_transport_read_u64(frame, offset++, &response->session_id);
  hubos_microkit_transport_read_u64(frame, offset++, &response->driver_id);
  hubos_microkit_transport_read_word(frame, offset++, &word);
  response->count = (size_t)word;
  hubos_microkit_transport_read_bool(frame, offset++, &response->is_new);
  hubos_microkit_transport_read_bool(frame, offset++, &response->bool_result);
  hubos_microkit_transport_read_word(frame, offset++, &word);
  response->boot_step = (hubos_boot_step_t)word;
  hubos_microkit_transport_read_u64(frame, offset++, &response->descriptor.resource_id);
  hubos_microkit_transport_read_ptr(frame, offset++, &ptr);
  response->descriptor.name = (const char *)ptr;
  hubos_microkit_transport_read_word(frame, offset++, &word);
  response->descriptor.name_len = (size_t)word;
  hubos_microkit_transport_read_word(frame, offset++, &word);
  response->descriptor.resource_state = (hubos_resource_state_t)word;
  hubos_microkit_transport_read_ptr(frame, offset++, &ptr);
  response->descriptor.endpoint = (const char *)ptr;
  hubos_microkit_transport_read_ptr(frame, offset++, &ptr);
  response->descriptor.version = (const char *)ptr;
  hubos_microkit_transport_read_word(frame, offset++, &word);
  response->descriptor.policy_hints = (unsigned)word;
  return true;
}

static inline bool hubos_microkit_transport_synthesize_response(
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  if (request == NULL || response == NULL) {
    return false;
  }

  *response = (hubos_microkit_ipc_response_t){0};
  response->status = HUBOS_IPC_STATUS_INVALID_ARGUMENT;
  response->resource_id = HUBOS_ID_INVALID;
  response->capability_id = HUBOS_ID_INVALID;
  response->session_id = HUBOS_ID_INVALID;
  response->driver_id = HUBOS_ID_INVALID;
  response->descriptor.resource_id = HUBOS_ID_INVALID;
  response->descriptor.name = NULL;
  response->descriptor.name_len = 0;
  response->descriptor.resource_state = HUBOS_RESOURCE_DISCOVERED;
  response->descriptor.endpoint = NULL;
  response->descriptor.version = NULL;
  response->descriptor.policy_hints = 0;

  switch (request->service) {
  case HUBOS_MICROKIT_COMPONENT_ROOT_TASK:
    switch ((hubos_root_task_operation_t)request->operation) {
    case HUBOS_ROOT_TASK_OP_BOOTSTRAP:
      response->status = HUBOS_IPC_STATUS_OK;
      return true;
    case HUBOS_ROOT_TASK_OP_COMPLETE_BOOT_STEP:
      response->status = HUBOS_IPC_STATUS_OK;
      response->boot_step = request->payload.boot_step.step;
      return true;
    case HUBOS_ROOT_TASK_OP_QUERY_BOOT_STEP:
      response->status = HUBOS_IPC_STATUS_OK;
      response->boot_step = request->payload.boot_step.step;
      response->bool_result = true;
      return true;
    case HUBOS_ROOT_TASK_OP_ADVANCE_CONTROL_PLANE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->boot_step = HUBOS_BOOT_RESOURCE_REGISTRY;
      response->bool_result = true;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_RESOURCE_REGISTRY:
    switch ((hubos_microkit_resource_operation_t)request->operation) {
    case HUBOS_MICROKIT_RESOURCE_OP_REGISTER:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.resource_register.name_len == 0
                                  ? 1
                                  : (hubos_id_t)request->payload.resource_register.name_len;
      response->is_new = true;
      response->descriptor.resource_id = response->resource_id;
      response->descriptor.name = request->payload.resource_register.name;
      response->descriptor.name_len = request->payload.resource_register.name_len;
      response->descriptor.resource_state = request->payload.resource_register.state;
      response->descriptor.endpoint = request->payload.resource_register.name;
      response->descriptor.version = NULL;
      response->descriptor.policy_hints = 0;
      return true;
    case HUBOS_MICROKIT_RESOURCE_OP_UPDATE_STATE:
    case HUBOS_MICROKIT_RESOURCE_OP_QUARANTINE:
    case HUBOS_MICROKIT_RESOURCE_OP_RETIRE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.resource_update_state.resource_id;
      return true;
    case HUBOS_MICROKIT_RESOURCE_OP_DESCRIBE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.resource_describe.resource_id;
      response->descriptor.resource_id = request->payload.resource_describe.resource_id;
      response->descriptor.name = NULL;
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_DISCOVERED;
      response->descriptor.endpoint = NULL;
      response->descriptor.version = NULL;
      response->descriptor.policy_hints = 0;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_CAPABILITY_MANAGER:
    switch ((hubos_microkit_capability_operation_t)request->operation) {
    case HUBOS_MICROKIT_CAPABILITY_OP_ISSUE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->capability_id = request->payload.capability_issue.resource_id + 1;
      return true;
    case HUBOS_MICROKIT_CAPABILITY_OP_COPY:
      response->status = HUBOS_IPC_STATUS_OK;
      response->capability_id = request->payload.capability_copy.source_capability_id + 1;
      return true;
    case HUBOS_MICROKIT_CAPABILITY_OP_MINT_FROM:
      response->status = HUBOS_IPC_STATUS_OK;
      response->capability_id = request->payload.capability_mint.source_capability_id + 1;
      return true;
    case HUBOS_MICROKIT_CAPABILITY_OP_TRANSFER:
      response->status = HUBOS_IPC_STATUS_OK;
      response->capability_id = request->payload.capability_transfer.capability_id;
      return true;
    case HUBOS_MICROKIT_CAPABILITY_OP_REVOKE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->capability_id = request->payload.capability_revoke.capability_id;
      return true;
    case HUBOS_MICROKIT_CAPABILITY_OP_AUTHORIZE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->bool_result = request->payload.capability_authorize.required_rights != 0;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_SESSION_MANAGER:
    switch ((hubos_microkit_session_operation_t)request->operation) {
    case HUBOS_MICROKIT_SESSION_OP_CREATE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->session_id = request->payload.session_create.owner_id + 1;
      return true;
    case HUBOS_MICROKIT_SESSION_OP_REFRESH_CONTEXT:
      response->status = HUBOS_IPC_STATUS_OK;
      response->session_id = request->payload.session_refresh_context.session_id;
      return true;
    case HUBOS_MICROKIT_SESSION_OP_SET_STATE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->session_id = request->payload.session_set_state.session_id;
      return true;
    case HUBOS_MICROKIT_SESSION_OP_REVOKE_TREE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->session_id = request->payload.session_revoke_tree.session_id;
      return true;
    case HUBOS_MICROKIT_SESSION_OP_IS_ANCESTOR:
      response->status = HUBOS_IPC_STATUS_OK;
      response->bool_result = request->payload.session_is_ancestor.ancestor_id <
                              request->payload.session_is_ancestor.session_id;
      return true;
    case HUBOS_MICROKIT_SESSION_OP_CHILD_COUNT:
      response->status = HUBOS_IPC_STATUS_OK;
      response->count = 0;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_HUB:
    switch ((hubos_microkit_hub_operation_t)request->operation) {
    case HUBOS_MICROKIT_HUB_OP_RESOLVE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.hub_resolve.name_len == 0
                                  ? 1
                                  : (hubos_id_t)request->payload.hub_resolve.name_len;
      response->descriptor.resource_id = response->resource_id;
      response->descriptor.name = request->payload.hub_resolve.name;
      response->descriptor.name_len = request->payload.hub_resolve.name_len;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = request->payload.hub_resolve.name;
      response->descriptor.version = "qemu";
      response->descriptor.policy_hints = 0;
      return true;
    case HUBOS_MICROKIT_HUB_OP_AUTHORIZE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->bool_result = request->payload.hub_authorize.required_rights != 0;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_DRIVER_SERVICE:
    switch ((hubos_microkit_driver_operation_t)request->operation) {
    case HUBOS_MICROKIT_DRIVER_OP_BIND:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.driver_bind.resource_id;
      response->driver_id = request->payload.driver_bind.driver_id;
      response->count = request->payload.driver_bind.package.dependency_count;
      return true;
    case HUBOS_MICROKIT_DRIVER_OP_REBIND:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.driver_rebind.resource_id;
      response->driver_id = request->payload.driver_rebind.driver_id;
      response->count = request->payload.driver_rebind.package.dependency_count;
      return true;
    case HUBOS_MICROKIT_DRIVER_OP_QUARANTINE:
    case HUBOS_MICROKIT_DRIVER_OP_UNBIND:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.driver_quarantine.resource_id;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER:
    switch ((hubos_microkit_network_operation_t)request->operation) {
    case HUBOS_MICROKIT_NETWORK_OP_BIND_NAMESPACE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.network_bind_namespace.namespace_handle.id;
      response->descriptor.resource_id = request->payload.network_bind_namespace.namespace_handle.id;
      response->descriptor.name = request->payload.network_bind_namespace.namespace_handle.name;
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = request->payload.network_bind_namespace.namespace_handle.name;
      response->descriptor.version = "network";
      response->descriptor.policy_hints =
        request->payload.network_bind_namespace.namespace_handle.owned_by_server ? 1u : 0u;
      return true;
    case HUBOS_MICROKIT_NETWORK_OP_SET_POLICY:
      response->status = HUBOS_IPC_STATUS_OK;
      response->bool_result = request->payload.network_set_policy.routing_enabled ||
                              request->payload.network_set_policy.firewall_enabled;
      return true;
    case HUBOS_MICROKIT_NETWORK_OP_ADD_ROUTE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.network_add_route.nic_resource_id;
      return true;
    case HUBOS_MICROKIT_NETWORK_OP_SET_DEFAULT_ROUTE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.network_set_default_route.nic_resource_id;
      return true;
    case HUBOS_MICROKIT_NETWORK_OP_SELECT_NIC:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.network_select_nic.destination_len == 0
                                  ? 0
                                  : request->payload.network_select_nic.destination_len;
      return true;
    case HUBOS_MICROKIT_NETWORK_OP_BIND_PORT:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.network_bind_port.nic_resource_id;
      response->session_id = request->payload.network_bind_port.session_id;
      return true;
    case HUBOS_MICROKIT_NETWORK_OP_SET_FAILOVER_POLICY:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.network_set_failover_policy.preferred_nic_resource_id;
      response->bool_result = request->payload.network_set_failover_policy.failover_enabled;
      return true;
    case HUBOS_MICROKIT_NETWORK_OP_DESCRIBE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.resource_id = HUBOS_MICROKIT_COMPONENT_NETWORK_SERVER;
      response->descriptor.name = "Network Server";
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = "Network Server";
      response->descriptor.version = "qemu";
      response->descriptor.policy_hints = 0;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER:
    switch ((hubos_microkit_storage_operation_t)request->operation) {
    case HUBOS_MICROKIT_STORAGE_OP_BIND_NAMESPACE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.storage_bind_namespace.namespace_handle.id;
      response->descriptor.resource_id = request->payload.storage_bind_namespace.namespace_handle.id;
      response->descriptor.name = request->payload.storage_bind_namespace.namespace_handle.name;
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = request->payload.storage_bind_namespace.namespace_handle.name;
      response->descriptor.version = "storage";
      response->descriptor.policy_hints =
        request->payload.storage_bind_namespace.namespace_handle.owned_by_server ? 1u : 0u;
      return true;
    case HUBOS_MICROKIT_STORAGE_OP_RELEASE_NAMESPACE:
      response->status = HUBOS_IPC_STATUS_OK;
      return true;
    case HUBOS_MICROKIT_STORAGE_OP_FINALIZE_NAMESPACE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.resource_state = HUBOS_RESOURCE_RETIRED;
      return true;
    case HUBOS_MICROKIT_STORAGE_OP_DESCRIBE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.resource_id = HUBOS_MICROKIT_COMPONENT_STORAGE_SERVER;
      response->descriptor.name = "Storage Server";
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = "Storage Server";
      response->descriptor.version = "qemu";
      response->descriptor.policy_hints = 0;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER:
    switch ((hubos_microkit_display_operation_t)request->operation) {
    case HUBOS_MICROKIT_DISPLAY_OP_BIND_NAMESPACE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.display_bind_namespace.namespace_handle.id;
      response->descriptor.resource_id = request->payload.display_bind_namespace.namespace_handle.id;
      response->descriptor.name = request->payload.display_bind_namespace.namespace_handle.name;
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = request->payload.display_bind_namespace.namespace_handle.name;
      response->descriptor.version = "display";
      response->descriptor.policy_hints =
        request->payload.display_bind_namespace.namespace_handle.owned_by_server ? 1u : 0u;
      return true;
    case HUBOS_MICROKIT_DISPLAY_OP_RELEASE_NAMESPACE:
      response->status = HUBOS_IPC_STATUS_OK;
      return true;
    case HUBOS_MICROKIT_DISPLAY_OP_FINALIZE_NAMESPACE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.resource_state = HUBOS_RESOURCE_RETIRED;
      return true;
    case HUBOS_MICROKIT_DISPLAY_OP_DESCRIBE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.resource_id = HUBOS_MICROKIT_COMPONENT_DISPLAY_SERVER;
      response->descriptor.name = "Display Server";
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = "Display Server";
      response->descriptor.version = "qemu";
      response->descriptor.policy_hints = 0;
      return true;
    default:
      return false;
    }
  case HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER:
    switch ((hubos_microkit_device_operation_t)request->operation) {
    case HUBOS_MICROKIT_DEVICE_OP_SET_OWNER:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_MMIO:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_IRQ:
    case HUBOS_MICROKIT_DEVICE_OP_ATTACH_DMA:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.device_owner.owner_session_id;
      response->descriptor.resource_id = HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER;
      response->descriptor.name = "Device Server";
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = "Device Server";
      response->descriptor.version = "qemu";
      response->descriptor.policy_hints = 0x7u;
      return true;
    case HUBOS_MICROKIT_DEVICE_OP_RELEASE_OWNER:
    case HUBOS_MICROKIT_DEVICE_OP_RESET:
    case HUBOS_MICROKIT_DEVICE_OP_QUARANTINE:
    case HUBOS_MICROKIT_DEVICE_OP_CLEAR_QUARANTINE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.resource_id = HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER;
      response->descriptor.name = "Device Server";
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = "Device Server";
      response->descriptor.version = "qemu";
      response->descriptor.policy_hints = 0;
      return true;
    case HUBOS_MICROKIT_DEVICE_OP_DESCRIBE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.resource_id = HUBOS_MICROKIT_COMPONENT_DEVICE_SERVER;
      response->descriptor.name = "Device Server";
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = "Device Server";
      response->descriptor.version = "qemu";
      response->descriptor.policy_hints = 0x7u;
      return true;
    default:
      return false;
    }
  default:
    break;
  case HUBOS_MICROKIT_COMPONENT_VM_SERVER:
    switch ((hubos_microkit_vm_operation_t)request->operation) {
    case HUBOS_MICROKIT_VM_OP_SET_GUEST_MEMORY:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = request->payload.vm_guest_memory.guest_memory_id;
      return true;
    case HUBOS_MICROKIT_VM_OP_SET_VCPU_COUNT:
      response->status = HUBOS_IPC_STATUS_OK;
      response->count = request->payload.vm_vcpu_count.vcpu_count;
      return true;
    case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_NET:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VIRTIO_BLK:
    case HUBOS_MICROKIT_VM_OP_ATTACH_VGPU:
      response->status = HUBOS_IPC_STATUS_OK;
      response->session_id = request->payload.vm_session.session_id;
      return true;
    case HUBOS_MICROKIT_VM_OP_SELECT_RUNTIME_PROFILE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.version = request->payload.vm_select_runtime_profile.runtime_profile_id;
      return true;
    case HUBOS_MICROKIT_VM_OP_SET_ARTIFACTS:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.version = request->payload.vm_set_artifacts.artifacts.kernel_image;
      return true;
    case HUBOS_MICROKIT_VM_OP_SET_RESTART_POLICY:
      response->status = HUBOS_IPC_STATUS_OK;
      response->count = request->payload.vm_set_restart_policy.max_restart_attempts;
      return true;
    case HUBOS_MICROKIT_VM_OP_START:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = HUBOS_MICROKIT_COMPONENT_VM_SERVER;
      response->descriptor.resource_state = HUBOS_RESOURCE_CLASSIFIED;
      response->descriptor.policy_hints = 512u;
      return true;
    case HUBOS_MICROKIT_VM_OP_COMPLETE_BOOT:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = HUBOS_MICROKIT_COMPONENT_VM_SERVER;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.policy_hints = 1024u;
      return true;
    case HUBOS_MICROKIT_VM_OP_FAIL:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = HUBOS_MICROKIT_COMPONENT_VM_SERVER;
      response->bool_result = false;
      response->descriptor.resource_state = HUBOS_RESOURCE_FAILED;
      response->descriptor.policy_hints = 2048u;
      return true;
    case HUBOS_MICROKIT_VM_OP_STOP:
      response->status = HUBOS_IPC_STATUS_OK;
      response->resource_id = HUBOS_MICROKIT_COMPONENT_VM_SERVER;
      response->descriptor.resource_state = HUBOS_RESOURCE_BOUND;
      response->descriptor.policy_hints = 256u;
      return true;
    case HUBOS_MICROKIT_VM_OP_DESCRIBE:
      response->status = HUBOS_IPC_STATUS_OK;
      response->descriptor.resource_id = HUBOS_MICROKIT_COMPONENT_VM_SERVER;
      response->descriptor.name = "VM Server";
      response->descriptor.name_len = 0;
      response->descriptor.resource_state = HUBOS_RESOURCE_READY;
      response->descriptor.endpoint = "VM Server";
      response->descriptor.version = "libvmm";
      response->descriptor.policy_hints = 0x1fu | 256u;
      return true;
    default:
      return false;
    }
  }

  return false;
}

static inline bool hubos_microkit_transport_request_reserve(
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_transport_frame_t *frame) {
  size_t expected = hubos_microkit_transport_request_word_count(request);

  if (expected == 0 || frame == NULL || expected > HUBOS_MICROKIT_TRANSPORT_MAX_WORDS) {
    return false;
  }

  frame->count = (seL4_Uint16)expected;
  return true;
}

#endif
