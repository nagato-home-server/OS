#ifndef HUBOS_ROOT_TASK_SHELL_H
#define HUBOS_ROOT_TASK_SHELL_H

#include "hubos/app_vm_runtime.h"
#include "hubos/capability_manager.h"
#include "hubos/driver_registry.h"
#include "hubos/driver_service.h"
#include "hubos/model.h"
#include "hubos/namespace.h"
#include "hubos/runtime_config.h"
#include "hubos/system.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  void (*puts_fn)(const char *text, void *context);
  void (*putu64_fn)(uint64_t value, void *context);
  void *context;
} hubos_root_task_shell_io_t;

static inline void hubos_root_task_shell_puts(const hubos_root_task_shell_io_t *io,
                                              const char *text) {
  if (io != NULL && io->puts_fn != NULL) {
    io->puts_fn(text, io->context);
  }
}

static inline void hubos_root_task_shell_putu64(const hubos_root_task_shell_io_t *io,
                                                uint64_t value) {
  if (io != NULL && io->putu64_fn != NULL) {
    io->putu64_fn(value, io->context);
  }
}

static inline bool hubos_root_task_shell_literal_equals(const char *lhs, const char *rhs) {
  if (lhs == NULL || rhs == NULL) {
    return false;
  }

  while (*lhs != '\0' && *rhs != '\0' && *lhs == *rhs) {
    ++lhs;
    ++rhs;
  }

  return *lhs == '\0' && *rhs == '\0';
}

static inline bool hubos_root_task_shell_parse_u64(const char *text, uint64_t *out_value) {
  uint64_t value = 0;

  if (text == NULL || text[0] == '\0' || out_value == NULL) {
    return false;
  }

  for (; *text != '\0'; ++text) {
    if (*text < '0' || *text > '9') {
      return false;
    }
    value = value * 10u + (uint64_t)(*text - '0');
  }

  *out_value = value;
  return true;
}

static inline bool hubos_root_task_shell_parse_bool(const char *text, bool *out_value) {
  if (out_value == NULL) {
    return false;
  }

  if (hubos_root_task_shell_literal_equals(text, "on") ||
      hubos_root_task_shell_literal_equals(text, "true") ||
      hubos_root_task_shell_literal_equals(text, "1")) {
    *out_value = true;
    return true;
  }

  if (hubos_root_task_shell_literal_equals(text, "off") ||
      hubos_root_task_shell_literal_equals(text, "false") ||
      hubos_root_task_shell_literal_equals(text, "0")) {
    *out_value = false;
    return true;
  }

  return false;
}

static inline const char *hubos_root_task_shell_session_type_name(hubos_session_type_t type) {
  switch (type) {
  case HUBOS_SESSION_PERMANENT:
    return "permanent";
  case HUBOS_SESSION_PERSISTENT:
    return "persistent";
  case HUBOS_SESSION_EPHEMERAL:
    return "ephemeral";
  case HUBOS_SESSION_TRANSACTIONAL:
    return "transactional";
  }

  return "unknown";
}

static inline bool hubos_root_task_shell_parse_session_type(const char *text,
                                                            hubos_session_type_t *out_type) {
  if (out_type == NULL) {
    return false;
  }

  if (hubos_root_task_shell_literal_equals(text, "permanent")) {
    *out_type = HUBOS_SESSION_PERMANENT;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "persistent")) {
    *out_type = HUBOS_SESSION_PERSISTENT;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "ephemeral")) {
    *out_type = HUBOS_SESSION_EPHEMERAL;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "transactional")) {
    *out_type = HUBOS_SESSION_TRANSACTIONAL;
    return true;
  }

  return false;
}

static inline bool hubos_root_task_shell_parse_session_state(const char *text,
                                                             hubos_session_state_t *out_state) {
  if (out_state == NULL) {
    return false;
  }

  if (hubos_root_task_shell_literal_equals(text, "created")) {
    *out_state = HUBOS_SESSION_CREATED;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "active")) {
    *out_state = HUBOS_SESSION_ACTIVE;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "draining")) {
    *out_state = HUBOS_SESSION_DRAINING;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "revoked")) {
    *out_state = HUBOS_SESSION_REVOKED;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "retired")) {
    *out_state = HUBOS_SESSION_RETIRED;
    return true;
  }

  return false;
}

static inline bool hubos_root_task_shell_parse_resource_state(const char *text,
                                                              hubos_resource_state_t *out_state) {
  if (out_state == NULL) {
    return false;
  }

  if (hubos_root_task_shell_literal_equals(text, "discovered")) {
    *out_state = HUBOS_RESOURCE_DISCOVERED;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "classified")) {
    *out_state = HUBOS_RESOURCE_CLASSIFIED;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "bound")) {
    *out_state = HUBOS_RESOURCE_BOUND;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "ready")) {
    *out_state = HUBOS_RESOURCE_READY;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "failed")) {
    *out_state = HUBOS_RESOURCE_FAILED;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "quarantined")) {
    *out_state = HUBOS_RESOURCE_QUARANTINED;
    return true;
  }
  if (hubos_root_task_shell_literal_equals(text, "retired")) {
    *out_state = HUBOS_RESOURCE_RETIRED;
    return true;
  }

  return false;
}

static inline void hubos_root_task_shell_write_kv(const hubos_root_task_shell_io_t *io,
                                                  const char *key,
                                                  const char *value) {
  hubos_root_task_shell_puts(io, key);
  hubos_root_task_shell_puts(io, value != NULL ? value : "(none)");
  hubos_root_task_shell_puts(io, "\n");
}

static inline void hubos_root_task_shell_write_kv_u64(const hubos_root_task_shell_io_t *io,
                                                      const char *key,
                                                      uint64_t value) {
  hubos_root_task_shell_puts(io, key);
  hubos_root_task_shell_putu64(io, value);
  hubos_root_task_shell_puts(io, "\n");
}

static inline void hubos_root_task_shell_print_descriptor(const hubos_root_task_shell_io_t *io,
                                                          const char *prefix,
                                                          const hubos_service_descriptor_t *descriptor) {
  if (descriptor == NULL) {
    hubos_root_task_shell_puts(io, "missing descriptor\n");
    return;
  }

  hubos_root_task_shell_write_kv(io, prefix, descriptor->name);
  hubos_root_task_shell_write_kv_u64(io, "resource_id=", descriptor->resource_id);
  hubos_root_task_shell_write_kv(io, "state=", hubos_resource_state_name(descriptor->resource_state));
  hubos_root_task_shell_write_kv(io, "endpoint=", descriptor->endpoint);
  hubos_root_task_shell_write_kv(io, "version=", descriptor->version);
  hubos_root_task_shell_write_kv_u64(io, "policy_hints=", descriptor->policy_hints);
}

static inline void hubos_root_task_shell_help(const hubos_root_task_shell_io_t *io) {
  hubos_root_task_shell_puts(
    io,
    "commands: help status services hub resolve <name> "
    "resource count|list|describe <id>|register <name>|set-state <id> <state>|quarantine <id>|retire <id> "
    "cap issue <owner> <resource> <rights> <delegatable>|describe <id>|authorize <cap> <resource> <rights>|revoke <id> "
    "session create <owner> <parent> <type>|describe <id>|set-state <id> <state>|child-count <id>|revoke-tree <id> "
    "driver status <resource>|bind <resource> <driver> <version>|rebind <resource> <driver> <version>|quarantine <resource>|unbind <resource> "
    "network status|policy <routing> <firewall>|route-add <destination> <nic> <metric>|default-route <nic>|select-nic <destination>|bind-port <port> <nic> <session>|failover <enabled> [preferred_nic] "
    "storage status|bind-ns <id> <owner> <name>|release|finalize "
    "display status|bind-ns <id> <owner> <name>|release|finalize "
    "device status|owner <session>|release|reset|quarantine|clear|attach-mmio <session>|attach-irq <session>|attach-dma <session> "
    "vm status|start|stop|restart|profile|profiles|profile <id>|vcpus <count>|guest-memory <id>|restart-policy manual|auto [count]|attach-net <session>|attach-blk <session>|attach-vgpu <session>|console status|attach|detach|write <token>|describe\n");
}

static inline void hubos_root_task_shell_print_vm_status(hubos_system_t *system,
                                                         const hubos_root_task_shell_io_t *io) {
  hubos_vm_server_t *vm = &system->vm_server;
  const char *state = "unknown";

  switch (vm->state) {
  case HUBOS_VM_STOPPED:
    state = "stopped";
    break;
  case HUBOS_VM_BOOTING:
    state = "booting";
    break;
  case HUBOS_VM_RUNNING:
    state = "running";
    break;
  case HUBOS_VM_FAILED:
    state = "failed";
    break;
  }

  hubos_root_task_shell_write_kv(io, "vm.state=", state);
  hubos_root_task_shell_write_kv(io,
                                 "vm.profile=",
                                 vm->runtime_profile != NULL ? vm->runtime_profile->id : "(none)");
  hubos_root_task_shell_write_kv_u64(io, "vm.vcpus=", vm->vm.vcpu_count);
  hubos_root_task_shell_write_kv_u64(io, "vm.guest_memory_id=", vm->vm.guest_memory_id);
  hubos_root_task_shell_write_kv_u64(io, "vm.virtio_net_session=", vm->vm.virtio_net_session_id);
  hubos_root_task_shell_write_kv_u64(io, "vm.virtio_blk_session=", vm->vm.virtio_blk_session_id);
  hubos_root_task_shell_write_kv_u64(io, "vm.vgpu_session=", vm->vm.vgpu_session_id);
  hubos_root_task_shell_write_kv(io,
                                 "vm.restart_policy=",
                                 vm->restart_policy == HUBOS_VM_RESTART_AUTO ? "auto" : "manual");
  hubos_root_task_shell_write_kv_u64(io, "vm.restart_attempts=", vm->restart_attempts);
  hubos_root_task_shell_write_kv_u64(io, "vm.max_restart_attempts=", vm->max_restart_attempts);
  hubos_root_task_shell_write_kv_u64(io, "vm.last_failure_code=", vm->last_failure_code);
}

static inline void hubos_root_task_shell_print_vm_console_status(hubos_system_t *system,
                                                                 const hubos_root_task_shell_io_t *io) {
  const hubos_vm_server_t *vm = &system->vm_server;

  hubos_root_task_shell_write_kv(io,
                                 "vm.console=",
                                 vm->console_relay_available ? "available" : "unavailable");
  hubos_root_task_shell_write_kv(io,
                                 "vm.console.attached=",
                                 vm->console_attached ? "true" : "false");
  hubos_root_task_shell_write_kv(io,
                                 "vm.console.backend=",
                                 vm->console_backend_name != NULL ?
                                   vm->console_backend_name :
                                   vm->backend_name);
  hubos_root_task_shell_write_kv_u64(io, "vm.console.tx_bytes=", vm->console_tx_bytes);
  hubos_root_task_shell_write_kv_u64(io, "vm.console.rx_bytes=", vm->console_rx_bytes);
  if (!vm->console_relay_available) {
    hubos_root_task_shell_write_kv(io, "vm.console.reason=", "guest serial relay not wired");
  } else if (vm->state != HUBOS_VM_RUNNING) {
    hubos_root_task_shell_write_kv(io, "vm.console.reason=", "vm not running");
  }
}

static inline void hubos_root_task_shell_print_services(hubos_system_t *system,
                                                        const hubos_root_task_shell_io_t *io) {
  hubos_service_descriptor_t descriptor = {0};

  if (hubos_system_describe_vm(system, &descriptor)) {
    hubos_root_task_shell_write_kv(io, "service.vm=", descriptor.name);
    hubos_root_task_shell_write_kv(io, "service.vm.state=", hubos_resource_state_name(descriptor.resource_state));
  }
  if (hubos_system_describe_network_server(system, &descriptor)) {
    hubos_root_task_shell_write_kv(io, "service.network=", descriptor.name);
    hubos_root_task_shell_write_kv(io, "service.network.state=", hubos_resource_state_name(descriptor.resource_state));
  }
  if (hubos_system_describe_storage_server(system, &descriptor)) {
    hubos_root_task_shell_write_kv(io, "service.storage=", descriptor.name);
    hubos_root_task_shell_write_kv(io, "service.storage.state=", hubos_resource_state_name(descriptor.resource_state));
  }
  if (hubos_system_describe_display_server(system, &descriptor)) {
    hubos_root_task_shell_write_kv(io, "service.display=", descriptor.name);
    hubos_root_task_shell_write_kv(io, "service.display.state=", hubos_resource_state_name(descriptor.resource_state));
  }
  if (hubos_system_describe_device(system, &descriptor)) {
    hubos_root_task_shell_write_kv(io, "service.device=", descriptor.name);
    hubos_root_task_shell_write_kv(io, "service.device.state=", hubos_resource_state_name(descriptor.resource_state));
  }
}

static inline void hubos_root_task_shell_vm_profiles(const hubos_root_task_shell_io_t *io) {
  size_t profile_count = 0;
  const hubos_app_vm_runtime_profile_t *profiles = hubos_runtime_config_profiles(&profile_count);

  hubos_root_task_shell_puts(io, "profiles:");
  if (profiles == NULL || profile_count == 0u) {
    hubos_root_task_shell_puts(io, " none\n");
    return;
  }

  for (size_t index = 0; index < profile_count; ++index) {
    hubos_root_task_shell_puts(io, index == 0u ? " " : ", ");
    hubos_root_task_shell_puts(io, profiles[index].id != NULL ? profiles[index].id : "(unnamed)");
  }
  hubos_root_task_shell_puts(io, "\n");
}

static inline bool hubos_root_task_shell_vm_select_profile(hubos_system_t *system,
                                                           const char *profile_id,
                                                           const hubos_root_task_shell_io_t *io) {
  size_t profile_count = 0;
  const hubos_app_vm_runtime_profile_t *profiles = hubos_runtime_config_profiles(&profile_count);
  const hubos_app_vm_runtime_profile_t *profile =
    hubos_app_vm_runtime_catalog_find(profiles, profile_count, profile_id);

  if (profile == NULL) {
    hubos_root_task_shell_puts(io, "unknown profile\n");
    return false;
  }

  if (!hubos_system_select_vm_runtime_profile(system, profile)) {
    hubos_root_task_shell_puts(io, "failed to select profile\n");
    return false;
  }

  hubos_root_task_shell_puts(io, "selected profile ");
  hubos_root_task_shell_puts(io, profile->id);
  hubos_root_task_shell_puts(io, "\n");
  return true;
}

static inline bool hubos_root_task_shell_execute(hubos_system_t *system,
                                                 int argc,
                                                 char **argv,
                                                 const hubos_root_task_shell_io_t *io) {
  uint64_t value0 = 0;
  uint64_t value1 = 0;
  uint64_t value2 = 0;
  bool flag0 = false;
  bool flag1 = false;
  hubos_service_descriptor_t descriptor = {0};

  if (system == NULL || io == NULL || argc <= 0 || argv == NULL) {
    return false;
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "help")) {
    hubos_root_task_shell_help(io);
    return true;
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "status")) {
    hubos_root_task_shell_print_vm_status(system, io);
    return true;
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "services")) {
    hubos_root_task_shell_print_services(system, io);
    return true;
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "hub") && argc == 3 &&
      hubos_root_task_shell_literal_equals(argv[1], "resolve")) {
    if (hubos_system_resolve(system, argv[2], 0, &descriptor)) {
      hubos_root_task_shell_print_descriptor(io, "hub.name=", &descriptor);
    } else {
      hubos_root_task_shell_puts(io, "hub resolve failed\n");
    }
    return true;
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "resource")) {
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "count")) {
      hubos_root_task_shell_write_kv_u64(io, "resource.count=", system->resource_registry.count);
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "list")) {
      for (size_t index = 0; index < system->resource_registry.count; ++index) {
        const hubos_resource_t *resource = &system->resource_registry.items[index];
        hubos_root_task_shell_puts(io, "resource.id=");
        hubos_root_task_shell_putu64(io, resource->id);
        hubos_root_task_shell_puts(io, " name=");
        hubos_root_task_shell_puts(io, resource->name);
        hubos_root_task_shell_puts(io, " state=");
        hubos_root_task_shell_puts(io, hubos_resource_state_name(resource->state));
        hubos_root_task_shell_puts(io, "\n");
      }
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "describe") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      const hubos_resource_t *resource = hubos_resource_registry_get(&system->resource_registry, value0);
      if (resource == NULL) {
        hubos_root_task_shell_puts(io, "resource not found\n");
      } else {
        hubos_root_task_shell_write_kv_u64(io, "resource.id=", resource->id);
        hubos_root_task_shell_write_kv(io, "resource.name=", resource->name);
        hubos_root_task_shell_write_kv(io, "resource.state=", hubos_resource_state_name(resource->state));
        hubos_root_task_shell_write_kv_u64(io, "resource.discovery_count=", resource->discovery_count);
      }
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "register")) {
      hubos_id_t resource_id = HUBOS_ID_INVALID;
      bool is_new = false;

      if (hubos_system_register_resource(system,
                                         argv[2],
                                         0,
                                         HUBOS_RESOURCE_DISCOVERED,
                                         &resource_id,
                                         &is_new)) {
        hubos_root_task_shell_write_kv_u64(io, "resource.id=", resource_id);
        hubos_root_task_shell_write_kv(io, "resource.new=", is_new ? "true" : "false");
      } else {
        hubos_root_task_shell_puts(io, "resource register failed\n");
      }
      return true;
    }
    if (argc == 4 && hubos_root_task_shell_literal_equals(argv[1], "set-state") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_resource_state_t state = HUBOS_RESOURCE_DISCOVERED;
      if (!hubos_root_task_shell_parse_resource_state(argv[3], &state)) {
        hubos_root_task_shell_puts(io, "invalid resource state\n");
      } else if (hubos_resource_registry_endpoint_update_state(&system->resource_registry_endpoint,
                                                               value0,
                                                               state)) {
        hubos_root_task_shell_puts(io, "resource state updated\n");
      } else {
        hubos_root_task_shell_puts(io, "resource set-state failed\n");
      }
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "quarantine") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_quarantine_resource(system, value0) ?
                                   "resource quarantined\n" :
                                   "resource quarantine failed\n");
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "retire") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_retire_resource(system, value0) ?
                                   "resource retired\n" :
                                   "resource retire failed\n");
      return true;
    }
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "cap")) {
    if (argc == 6 && hubos_root_task_shell_literal_equals(argv[1], "issue") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0) &&
        hubos_root_task_shell_parse_u64(argv[3], &value1) &&
        hubos_root_task_shell_parse_u64(argv[4], &value2) &&
        hubos_root_task_shell_parse_bool(argv[5], &flag0)) {
      hubos_id_t cap_id = HUBOS_ID_INVALID;
      if (hubos_system_issue_capability(system, value0, value1, (unsigned)value2, flag0, &cap_id)) {
        hubos_root_task_shell_write_kv_u64(io, "cap.id=", cap_id);
      } else {
        hubos_root_task_shell_puts(io, "cap issue failed\n");
      }
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "describe") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      const hubos_capability_t *capability =
        hubos_capability_manager_get(&system->capability_manager, value0);
      if (capability == NULL) {
        hubos_root_task_shell_puts(io, "capability not found\n");
      } else {
        hubos_root_task_shell_write_kv_u64(io, "cap.id=", capability->id);
        hubos_root_task_shell_write_kv_u64(io, "cap.owner_session=", capability->owner_session_id);
        hubos_root_task_shell_write_kv_u64(io, "cap.resource_id=", capability->resource_id);
        hubos_root_task_shell_write_kv_u64(io, "cap.rights=", capability->rights);
        hubos_root_task_shell_write_kv(io, "cap.delegatable=", capability->delegatable ? "true" : "false");
        hubos_root_task_shell_write_kv(io, "cap.revoked=", capability->revoked ? "true" : "false");
      }
      return true;
    }
    if (argc == 5 && hubos_root_task_shell_literal_equals(argv[1], "authorize") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0) &&
        hubos_root_task_shell_parse_u64(argv[3], &value1) &&
        hubos_root_task_shell_parse_u64(argv[4], &value2)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_authorize(system, value0, value1, (unsigned)value2) ?
                                   "cap authorize ok\n" :
                                   "cap authorize denied\n");
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "revoke") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_revoke_capability(system, value0) ?
                                   "cap revoked\n" :
                                   "cap revoke failed\n");
      return true;
    }
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "session")) {
    if (argc == 5 && hubos_root_task_shell_literal_equals(argv[1], "create") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0) &&
        hubos_root_task_shell_parse_u64(argv[3], &value1)) {
      hubos_session_type_t type = HUBOS_SESSION_EPHEMERAL;
      hubos_id_t session_id = HUBOS_ID_INVALID;
      if (!hubos_root_task_shell_parse_session_type(argv[4], &type)) {
        hubos_root_task_shell_puts(io, "invalid session type\n");
      } else if (hubos_system_create_session(system, value0, value1, type, &session_id)) {
        hubos_root_task_shell_write_kv_u64(io, "session.id=", session_id);
      } else {
        hubos_root_task_shell_puts(io, "session create failed\n");
      }
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "describe") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      const hubos_session_t *session = hubos_session_manager_get(&system->session_manager, value0);
      if (session == NULL) {
        hubos_root_task_shell_puts(io, "session not found\n");
      } else {
        hubos_root_task_shell_write_kv_u64(io, "session.id=", session->id);
        hubos_root_task_shell_write_kv_u64(io, "session.owner=", session->owner_id);
        hubos_root_task_shell_write_kv_u64(io, "session.parent=", session->parent_id);
        hubos_root_task_shell_write_kv(io, "session.type=", hubos_root_task_shell_session_type_name(session->type));
        hubos_root_task_shell_write_kv(io, "session.state=", hubos_session_state_name(session->state));
        hubos_root_task_shell_write_kv_u64(io, "session.namespace_view_version=", session->namespace_view_version);
        hubos_root_task_shell_write_kv_u64(io, "session.policy_context_version=", session->policy_context_version);
      }
      return true;
    }
    if (argc == 4 && hubos_root_task_shell_literal_equals(argv[1], "set-state") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_session_state_t state = HUBOS_SESSION_CREATED;
      if (!hubos_root_task_shell_parse_session_state(argv[3], &state)) {
        hubos_root_task_shell_puts(io, "invalid session state\n");
      } else if (hubos_session_manager_endpoint_set_state(&system->session_manager_endpoint,
                                                          value0,
                                                          state)) {
        hubos_root_task_shell_puts(io, "session state updated\n");
      } else {
        hubos_root_task_shell_puts(io, "session set-state failed\n");
      }
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "child-count") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_write_kv_u64(io,
                                         "session.child_count=",
                                         hubos_session_manager_child_count(&system->session_manager, value0));
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "revoke-tree") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_revoke_session_tree(system, value0) ?
                                   "session tree revoked\n" :
                                   "session revoke-tree failed\n");
      return true;
    }
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "driver")) {
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "status") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      const hubos_driver_binding_t *binding = hubos_driver_service_get(&system->driver_service, value0);
      if (binding == NULL) {
        hubos_root_task_shell_puts(io, "driver binding not found\n");
      } else {
        hubos_root_task_shell_write_kv_u64(io, "driver.resource_id=", binding->resource_id);
        hubos_root_task_shell_write_kv_u64(io, "driver.driver_id=", binding->driver_id);
        hubos_root_task_shell_write_kv_u64(io, "driver.state=", binding->state);
      }
      return true;
    }
    if (argc == 5 &&
        (hubos_root_task_shell_literal_equals(argv[1], "bind") ||
         hubos_root_task_shell_literal_equals(argv[1], "rebind")) &&
        hubos_root_task_shell_parse_u64(argv[2], &value0) &&
        hubos_root_task_shell_parse_u64(argv[3], &value1)) {
      hubos_driver_package_t package = {0};
      package.version = argv[4];
      if (hubos_root_task_shell_literal_equals(argv[1], "bind")) {
        hubos_root_task_shell_puts(io,
                                   hubos_system_bind_driver(system, value0, value1, &package) ?
                                     "driver bound\n" :
                                     "driver bind failed\n");
      } else {
        hubos_root_task_shell_puts(io,
                                   hubos_system_rebind_driver(system, value0, value1, &package) ?
                                     "driver rebound\n" :
                                     "driver rebind failed\n");
      }
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "quarantine") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_quarantine_driver(system, value0) ?
                                   "driver quarantined\n" :
                                   "driver quarantine failed\n");
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "unbind") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_unbind_driver(system, value0) ?
                                   "driver unbound\n" :
                                   "driver unbind failed\n");
      return true;
    }
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "network")) {
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "status")) {
      if (hubos_system_describe_network_server(system, &descriptor)) {
        hubos_root_task_shell_print_descriptor(io, "network.name=", &descriptor);
      } else {
        hubos_root_task_shell_puts(io, "network describe failed\n");
      }
      return true;
    }
    if (argc == 4 && hubos_root_task_shell_literal_equals(argv[1], "policy") &&
        hubos_root_task_shell_parse_bool(argv[2], &flag0) &&
        hubos_root_task_shell_parse_bool(argv[3], &flag1)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_set_network_policy(system, flag0, flag1) ?
                                   "network policy updated\n" :
                                   "network policy failed\n");
      return true;
    }
    if (argc == 5 && hubos_root_task_shell_literal_equals(argv[1], "route-add") &&
        hubos_root_task_shell_parse_u64(argv[3], &value0) &&
        hubos_root_task_shell_parse_u64(argv[4], &value1)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_add_network_route(system, argv[2], value0, (unsigned)value1) ?
                                   "network route added\n" :
                                   "network route add failed\n");
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "default-route") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_set_network_default_route(system, value0) ?
                                   "network default route set\n" :
                                   "network default route failed\n");
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "select-nic")) {
      hubos_id_t nic_id = HUBOS_ID_INVALID;
      if (hubos_system_select_network_nic(system, argv[2], 0, &nic_id)) {
        hubos_root_task_shell_write_kv_u64(io, "network.nic=", nic_id);
      } else {
        hubos_root_task_shell_puts(io, "network select-nic failed\n");
      }
      return true;
    }
    if (argc == 5 && hubos_root_task_shell_literal_equals(argv[1], "bind-port") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0) &&
        hubos_root_task_shell_parse_u64(argv[3], &value1) &&
        hubos_root_task_shell_parse_u64(argv[4], &value2)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_bind_network_port(system, (unsigned)value0, value1, value2) ?
                                   "network port bound\n" :
                                   "network bind-port failed\n");
      return true;
    }
    if ((argc == 3 || argc == 4) && hubos_root_task_shell_literal_equals(argv[1], "failover") &&
        hubos_root_task_shell_parse_bool(argv[2], &flag0)) {
      value0 = argc == 4 && hubos_root_task_shell_parse_u64(argv[3], &value0) ? value0 : HUBOS_ID_INVALID;
      hubos_root_task_shell_puts(io,
                                 hubos_system_set_network_failover_policy(system, flag0, value0) ?
                                   "network failover updated\n" :
                                   "network failover failed\n");
      return true;
    }
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "storage") ||
      hubos_root_task_shell_literal_equals(argv[0], "display")) {
    bool is_storage = hubos_root_task_shell_literal_equals(argv[0], "storage");

    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "status")) {
      bool ok = is_storage ?
        hubos_system_describe_storage_server(system, &descriptor) :
        hubos_system_describe_display_server(system, &descriptor);
      if (ok) {
        hubos_root_task_shell_print_descriptor(io,
                                               is_storage ? "storage.name=" : "display.name=",
                                               &descriptor);
      } else {
        hubos_root_task_shell_puts(io, is_storage ? "storage describe failed\n" : "display describe failed\n");
      }
      return true;
    }
    if (argc == 5 && hubos_root_task_shell_literal_equals(argv[1], "bind-ns") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0) &&
        hubos_root_task_shell_parse_u64(argv[3], &value1)) {
      hubos_namespace_handle_t handle;
      hubos_namespace_handle_init(&handle,
                                  value0,
                                  is_storage ? HUBOS_NAMESPACE_STORAGE : HUBOS_NAMESPACE_DISPLAY,
                                  argv[4],
                                  false);
      hubos_namespace_handle_set_owner_session(&handle, value1);
      hubos_root_task_shell_puts(io,
                                 (is_storage ? hubos_system_bind_storage_namespace(system, handle)
                                             : hubos_system_bind_display_namespace(system, handle)) ?
                                   "namespace bound\n" :
                                   "namespace bind failed\n");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "release")) {
      hubos_root_task_shell_puts(io,
                                 (is_storage ? hubos_system_release_storage_namespace(system)
                                             : hubos_system_release_display_namespace(system)) ?
                                   "namespace released\n" :
                                   "namespace release failed\n");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "finalize")) {
      hubos_root_task_shell_puts(io,
                                 (is_storage ? hubos_system_finalize_storage_namespace(system)
                                             : hubos_system_finalize_display_namespace(system)) ?
                                   "namespace finalized\n" :
                                   "namespace finalize failed\n");
      return true;
    }
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "device")) {
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "status")) {
      if (hubos_system_describe_device(system, &descriptor)) {
        hubos_root_task_shell_print_descriptor(io, "device.name=", &descriptor);
        hubos_root_task_shell_write_kv_u64(io, "device.owner_session=", system->device_server.owner_session_id);
        hubos_root_task_shell_write_kv_u64(io, "device.mmio_owner_session=", system->device_server.mmio_owner_session_id);
        hubos_root_task_shell_write_kv_u64(io, "device.irq_owner_session=", system->device_server.irq_owner_session_id);
        hubos_root_task_shell_write_kv_u64(io, "device.dma_owner_session=", system->device_server.dma_owner_session_id);
      } else {
        hubos_root_task_shell_puts(io, "device describe failed\n");
      }
      return true;
    }
    if (argc == 3 &&
        (hubos_root_task_shell_literal_equals(argv[1], "owner") ||
         hubos_root_task_shell_literal_equals(argv[1], "attach-mmio") ||
         hubos_root_task_shell_literal_equals(argv[1], "attach-irq") ||
         hubos_root_task_shell_literal_equals(argv[1], "attach-dma")) &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      bool ok = false;
      if (hubos_root_task_shell_literal_equals(argv[1], "owner")) {
        ok = hubos_system_set_device_owner(system, value0);
      } else if (hubos_root_task_shell_literal_equals(argv[1], "attach-mmio")) {
        ok = hubos_system_attach_device_mmio(system, value0);
      } else if (hubos_root_task_shell_literal_equals(argv[1], "attach-irq")) {
        ok = hubos_system_attach_device_irq(system, value0);
      } else {
        ok = hubos_system_attach_device_dma(system, value0);
      }
      hubos_root_task_shell_puts(io, ok ? "device updated\n" : "device update failed\n");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "release")) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_release_device_owner(system) ?
                                   "device owner released\n" :
                                   "device release failed\n");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "reset")) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_reset_device(system) ?
                                   "device reset\n" :
                                   "device reset failed\n");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "quarantine")) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_quarantine_device(system) ?
                                   "device quarantined\n" :
                                   "device quarantine failed\n");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "clear")) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_clear_device_quarantine(system) ?
                                   "device quarantine cleared\n" :
                                   "device clear failed\n");
      return true;
    }
  }

  if (hubos_root_task_shell_literal_equals(argv[0], "vm")) {
    if (argc == 2 &&
        (hubos_root_task_shell_literal_equals(argv[1], "status") ||
         hubos_root_task_shell_literal_equals(argv[1], "describe"))) {
      hubos_root_task_shell_print_vm_status(system, io);
      if (hubos_system_describe_vm(system, &descriptor)) {
        hubos_root_task_shell_print_descriptor(io, "vm.name=", &descriptor);
      }
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "console")) {
      hubos_root_task_shell_print_vm_console_status(system, io);
      return true;
    }
    if (hubos_root_task_shell_literal_equals(argv[1], "console")) {
      if (argc == 3 && hubos_root_task_shell_literal_equals(argv[2], "status")) {
        hubos_root_task_shell_print_vm_console_status(system, io);
        return true;
      }
      if (argc == 3 && hubos_root_task_shell_literal_equals(argv[2], "attach")) {
        if (hubos_system_attach_vm_console(system)) {
          hubos_root_task_shell_puts(io, "vm console attached\n");
        } else if (!hubos_system_vm_console_relay_available(system)) {
          hubos_root_task_shell_puts(io, "vm console attach failed: guest serial relay unavailable\n");
        } else {
          hubos_root_task_shell_puts(io, "vm console attach failed\n");
        }
        return true;
      }
      if (argc == 3 && hubos_root_task_shell_literal_equals(argv[2], "detach")) {
        hubos_root_task_shell_puts(io,
                                   hubos_system_detach_vm_console(system) ?
                                     "vm console detached\n" :
                                     "vm console detach failed\n");
        return true;
      }
      if (argc == 4 && hubos_root_task_shell_literal_equals(argv[2], "write")) {
        hubos_root_task_shell_puts(io,
                                   hubos_system_write_vm_console(system, argv[3], 0) ?
                                     "vm console write queued\n" :
                                     "vm console write failed\n");
        return true;
      }
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "start")) {
      if (system->vm_server.state == HUBOS_VM_RUNNING) {
        hubos_root_task_shell_puts(io, "vm already running\n");
      } else if (hubos_system_start_vm(system) && hubos_system_complete_vm_boot(system)) {
        hubos_root_task_shell_puts(io, "vm started\n");
      } else {
        hubos_root_task_shell_puts(io, "vm start failed\n");
      }
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "stop")) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_stop_vm(system) ? "vm stopped\n" : "vm stop failed\n");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "restart")) {
      (void)hubos_system_stop_vm(system);
      hubos_root_task_shell_puts(io,
                                 (hubos_system_start_vm(system) &&
                                  hubos_system_complete_vm_boot(system)) ?
                                   "vm restarted\n" :
                                   "vm restart failed\n");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "profile")) {
      hubos_root_task_shell_write_kv(io,
                                     "current profile ",
                                     system->vm_server.runtime_profile != NULL ?
                                       system->vm_server.runtime_profile->id :
                                       "(none)");
      return true;
    }
    if (argc == 2 && hubos_root_task_shell_literal_equals(argv[1], "profiles")) {
      hubos_root_task_shell_vm_profiles(io);
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "profile")) {
      (void)hubos_root_task_shell_vm_select_profile(system, argv[2], io);
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "vcpus") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_set_vm_vcpu_count(system, (unsigned)value0) ?
                                   "vm vcpu count updated\n" :
                                   "vm vcpu update failed\n");
      return true;
    }
    if (argc == 3 && hubos_root_task_shell_literal_equals(argv[1], "guest-memory") &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      hubos_root_task_shell_puts(io,
                                 hubos_system_set_vm_guest_memory(system, value0) ?
                                   "vm guest memory updated\n" :
                                   "vm guest memory update failed\n");
      return true;
    }
    if (argc == 3 &&
        (hubos_root_task_shell_literal_equals(argv[1], "attach-net") ||
         hubos_root_task_shell_literal_equals(argv[1], "attach-blk") ||
         hubos_root_task_shell_literal_equals(argv[1], "attach-vgpu")) &&
        hubos_root_task_shell_parse_u64(argv[2], &value0)) {
      bool ok = false;
      if (hubos_root_task_shell_literal_equals(argv[1], "attach-net")) {
        ok = hubos_system_attach_vm_virtio_net(system, value0);
      } else if (hubos_root_task_shell_literal_equals(argv[1], "attach-blk")) {
        ok = hubos_system_attach_vm_virtio_blk(system, value0);
      } else {
        ok = hubos_system_attach_vm_vgpu(system, value0);
      }
      hubos_root_task_shell_puts(io, ok ? "vm attachment updated\n" : "vm attachment failed\n");
      return true;
    }
    if ((argc == 3 || argc == 4) && hubos_root_task_shell_literal_equals(argv[1], "restart-policy")) {
      unsigned max_attempts = 0;
      hubos_vm_restart_policy_t policy = HUBOS_VM_RESTART_MANUAL;
      if (hubos_root_task_shell_literal_equals(argv[2], "manual")) {
        policy = HUBOS_VM_RESTART_MANUAL;
      } else if (hubos_root_task_shell_literal_equals(argv[2], "auto")) {
        policy = HUBOS_VM_RESTART_AUTO;
        if (argc != 4 || !hubos_root_task_shell_parse_u64(argv[3], &value0)) {
          hubos_root_task_shell_puts(io, "missing restart attempt count\n");
          return true;
        }
        max_attempts = (unsigned)value0;
      } else {
        hubos_root_task_shell_puts(io, "invalid restart policy\n");
        return true;
      }
      hubos_root_task_shell_puts(io,
                                 hubos_system_set_vm_restart_policy(system, policy, max_attempts) ?
                                   "vm restart policy updated\n" :
                                   "vm restart policy failed\n");
      return true;
    }
  }

  hubos_root_task_shell_puts(io, "unknown command\n");
  return true;
}

#endif
