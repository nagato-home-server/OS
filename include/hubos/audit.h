#ifndef HUBOS_AUDIT_H
#define HUBOS_AUDIT_H

#include "hubos/model.h"

typedef enum {
  HUBOS_AUDIT_RESOURCE_REGISTERED = 0,
  HUBOS_AUDIT_RESOURCE_DISCOVERED,
  HUBOS_AUDIT_RESOURCE_STATE_CHANGED,
  HUBOS_AUDIT_CAPABILITY_ISSUED,
  HUBOS_AUDIT_CAPABILITY_MINTED,
  HUBOS_AUDIT_CAPABILITY_TRANSFERRED,
  HUBOS_AUDIT_CAPABILITY_REVOKED,
  HUBOS_AUDIT_SESSION_CREATED,
  HUBOS_AUDIT_SESSION_DESTROYED,
  HUBOS_AUDIT_DRIVER_BOUND,
  HUBOS_AUDIT_DRIVER_REBIND_PREPARED,
  HUBOS_AUDIT_DRIVER_REBOUND,
  HUBOS_AUDIT_DRIVER_UNBOUND,
  HUBOS_AUDIT_DRIVER_QUARANTINED,
  HUBOS_AUDIT_DRIVER_KEY_ROTATED,
  HUBOS_AUDIT_DRIVER_KEY_REVOKED,
  HUBOS_AUDIT_DMA_MAPPED,
  HUBOS_AUDIT_DMA_REVOKED,
  HUBOS_AUDIT_DMA_ABORTED,
  HUBOS_AUDIT_BOOT_STEP_COMPLETED,
  HUBOS_AUDIT_VM_START_REQUESTED,
  HUBOS_AUDIT_VM_BOOT_COMPLETED,
  HUBOS_AUDIT_VM_FAILED,
  HUBOS_AUDIT_VM_STOPPED,
  HUBOS_AUDIT_VM_RESTART_SCHEDULED,
  HUBOS_AUDIT_VM_RESTART_POLICY_CHANGED,
} hubos_audit_event_type_t;

typedef struct {
  uint64_t sequence;
  hubos_audit_event_type_t type;
  hubos_id_t subject_id;
  hubos_id_t resource_id;
  hubos_id_t related_id;
  unsigned value;
} hubos_audit_event_t;

typedef struct {
  hubos_audit_event_t *items;
  size_t count;
  size_t capacity;
  uint64_t next_sequence;
} hubos_audit_log_t;

void hubos_audit_log_init(hubos_audit_log_t *log);
void hubos_audit_log_destroy(hubos_audit_log_t *log);

bool hubos_audit_log_record(hubos_audit_log_t *log,
                            hubos_audit_event_type_t type,
                            hubos_id_t subject_id,
                            hubos_id_t resource_id,
                            hubos_id_t related_id,
                            unsigned value);

const hubos_audit_event_t *hubos_audit_log_get(const hubos_audit_log_t *log, size_t index);
size_t hubos_audit_log_count(const hubos_audit_log_t *log);
void hubos_audit_log_clear(hubos_audit_log_t *log);

#endif
