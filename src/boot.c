#include "hubos/boot.h"

static bool hubos_boot_capability_kind_valid(hubos_boot_capability_kind_t kind) {
  return kind >= HUBOS_BOOT_CAP_FIRMWARE && kind < HUBOS_BOOT_CAP_COUNT;
}

void hubos_boot_state_init(hubos_boot_state_t *state) {
  if (state == NULL) {
    return;
  }

  for (size_t index = 0; index < HUBOS_BOOT_STEP_COUNT; ++index) {
    state->completed[index] = false;
  }
}

const char *hubos_boot_step_name(hubos_boot_step_t step) {
  switch (step) {
  case HUBOS_BOOT_FIRMWARE:
    return "firmware";
  case HUBOS_BOOT_SEL4:
    return "sel4";
  case HUBOS_BOOT_ROOT_TASK:
    return "root-task";
  case HUBOS_BOOT_RESOURCE_REGISTRY:
    return "resource-registry";
  case HUBOS_BOOT_SESSION_MANAGER:
    return "session-manager";
  case HUBOS_BOOT_CAPABILITY_MANAGER:
    return "capability-manager";
  case HUBOS_BOOT_MEMORY_MANAGER:
    return "memory-manager";
  case HUBOS_BOOT_DMA_MANAGER:
    return "dma-manager";
  case HUBOS_BOOT_HUB:
    return "hub";
  case HUBOS_BOOT_DRIVER_REGISTRY:
    return "driver-registry";
  case HUBOS_BOOT_BUS_MANAGERS:
    return "bus-managers";
  case HUBOS_BOOT_DEVICE_DISCOVERY:
    return "device-discovery";
  case HUBOS_BOOT_DRIVER_BINDING:
    return "driver-binding";
  case HUBOS_BOOT_SYSTEM_SERVERS:
    return "system-servers";
  case HUBOS_BOOT_APP_MANAGERS:
    return "app-managers";
  case HUBOS_BOOT_APPS:
    return "apps";
  case HUBOS_BOOT_STEP_COUNT:
    return "step-count";
  }

  return "unknown";
}

static bool hubos_boot_state_can_complete(const hubos_boot_state_t *state, hubos_boot_step_t step) {
  if (state == NULL || step >= HUBOS_BOOT_STEP_COUNT) {
    return false;
  }

  for (hubos_boot_step_t index = 0; index < step; ++index) {
    if (!state->completed[index]) {
      return false;
    }
  }

  return true;
}

bool hubos_boot_state_complete_step(hubos_boot_state_t *state,
                                    hubos_audit_log_t *audit_log,
                                    hubos_boot_step_t step) {
  if (state == NULL || step >= HUBOS_BOOT_STEP_COUNT) {
    return false;
  }

  if (!hubos_boot_state_can_complete(state, step)) {
    return false;
  }

  state->completed[step] = true;
  if (audit_log != NULL) {
    (void)hubos_audit_log_record(audit_log,
                                 HUBOS_AUDIT_BOOT_STEP_COMPLETED,
                                 0,
                                 0,
                                 0,
                                 (unsigned)step);
  }
  return true;
}

bool hubos_boot_state_is_complete(const hubos_boot_state_t *state, hubos_boot_step_t step) {
  if (state == NULL || step >= HUBOS_BOOT_STEP_COUNT) {
    return false;
  }

  return state->completed[step];
}

void hubos_boot_capability_set_init(hubos_boot_capability_set_t *set) {
  if (set == NULL) {
    return;
  }

  for (size_t index = 0; index < HUBOS_BOOT_CAP_COUNT; ++index) {
    set->granted[index] = false;
  }
}

bool hubos_boot_capability_set_grant(hubos_boot_capability_set_t *set,
                                     hubos_boot_capability_kind_t kind) {
  if (set == NULL || !hubos_boot_capability_kind_valid(kind)) {
    return false;
  }

  set->granted[kind] = true;
  return true;
}

bool hubos_boot_capability_set_has(const hubos_boot_capability_set_t *set,
                                   hubos_boot_capability_kind_t kind) {
  if (set == NULL || !hubos_boot_capability_kind_valid(kind)) {
    return false;
  }

  return set->granted[kind];
}

size_t hubos_boot_capability_set_count(const hubos_boot_capability_set_t *set) {
  size_t count = 0;

  if (set == NULL) {
    return 0;
  }

  for (size_t index = 0; index < HUBOS_BOOT_CAP_COUNT; ++index) {
    if (set->granted[index]) {
      ++count;
    }
  }

  return count;
}

bool hubos_boot_capability_set_validate_minimal(const hubos_boot_capability_set_t *set) {
  if (set == NULL) {
    return false;
  }

  for (size_t index = 0; index < HUBOS_BOOT_CAP_COUNT; ++index) {
    if (!set->granted[index]) {
      return false;
    }
  }

  return hubos_boot_capability_set_count(set) == HUBOS_BOOT_CAP_COUNT;
}
