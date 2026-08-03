#ifndef HUBOS_BOOT_H
#define HUBOS_BOOT_H

#include "hubos/audit.h"

typedef enum {
  HUBOS_BOOT_FIRMWARE = 0,
  HUBOS_BOOT_SEL4,
  HUBOS_BOOT_ROOT_TASK,
  HUBOS_BOOT_RESOURCE_REGISTRY,
  HUBOS_BOOT_SESSION_MANAGER,
  HUBOS_BOOT_CAPABILITY_MANAGER,
  HUBOS_BOOT_MEMORY_MANAGER,
  HUBOS_BOOT_DMA_MANAGER,
  HUBOS_BOOT_HUB,
  HUBOS_BOOT_DRIVER_REGISTRY,
  HUBOS_BOOT_BUS_MANAGERS,
  HUBOS_BOOT_DEVICE_DISCOVERY,
  HUBOS_BOOT_DRIVER_BINDING,
  HUBOS_BOOT_SYSTEM_SERVERS,
  HUBOS_BOOT_APP_MANAGERS,
  HUBOS_BOOT_APPS,
  HUBOS_BOOT_STEP_COUNT,
} hubos_boot_step_t;

typedef enum {
  HUBOS_BOOT_CAP_FIRMWARE = 0,
  HUBOS_BOOT_CAP_SEL4,
  HUBOS_BOOT_CAP_ROOT_TASK,
  HUBOS_BOOT_CAP_RESOURCE_REGISTRY,
  HUBOS_BOOT_CAP_CAPABILITY_MANAGER,
  HUBOS_BOOT_CAP_MEMORY_MANAGER,
  HUBOS_BOOT_CAP_DMA_MANAGER,
  HUBOS_BOOT_CAP_HUB,
  HUBOS_BOOT_CAP_COUNT,
} hubos_boot_capability_kind_t;

typedef struct {
  bool completed[HUBOS_BOOT_STEP_COUNT];
} hubos_boot_state_t;

typedef struct {
  bool granted[HUBOS_BOOT_CAP_COUNT];
} hubos_boot_capability_set_t;

void hubos_boot_state_init(hubos_boot_state_t *state);
const char *hubos_boot_step_name(hubos_boot_step_t step);
bool hubos_boot_state_complete_step(hubos_boot_state_t *state,
                                    hubos_audit_log_t *audit_log,
                                    hubos_boot_step_t step);
bool hubos_boot_state_is_complete(const hubos_boot_state_t *state, hubos_boot_step_t step);

void hubos_boot_capability_set_init(hubos_boot_capability_set_t *set);
bool hubos_boot_capability_set_grant(hubos_boot_capability_set_t *set,
                                     hubos_boot_capability_kind_t kind);
bool hubos_boot_capability_set_has(const hubos_boot_capability_set_t *set,
                                   hubos_boot_capability_kind_t kind);
size_t hubos_boot_capability_set_count(const hubos_boot_capability_set_t *set);
bool hubos_boot_capability_set_validate_minimal(const hubos_boot_capability_set_t *set);

#endif
