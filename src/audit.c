#include "hubos/audit.h"

#include <stdlib.h>

static bool hubos_audit_log_reserve(hubos_audit_log_t *log, size_t desired_capacity) {
  if (log == NULL) {
    return false;
  }

  if (log->capacity >= desired_capacity) {
    return true;
  }

  size_t new_capacity = log->capacity == 0 ? 8 : log->capacity;
  while (new_capacity < desired_capacity) {
    new_capacity *= 2;
  }

  void *new_items = realloc(log->items, new_capacity * sizeof(*log->items));
  if (new_items == NULL) {
    return false;
  }

  log->items = new_items;
  log->capacity = new_capacity;
  return true;
}

void hubos_audit_log_init(hubos_audit_log_t *log) {
  if (log == NULL) {
    return;
  }

  log->items = NULL;
  log->count = 0;
  log->capacity = 0;
  log->next_sequence = 1;
}

void hubos_audit_log_destroy(hubos_audit_log_t *log) {
  if (log == NULL) {
    return;
  }

  free(log->items);
  log->items = NULL;
  log->count = 0;
  log->capacity = 0;
  log->next_sequence = 1;
}

bool hubos_audit_log_record(hubos_audit_log_t *log,
                            hubos_audit_event_type_t type,
                            hubos_id_t subject_id,
                            hubos_id_t resource_id,
                            hubos_id_t related_id,
                            unsigned value) {
  hubos_audit_event_t event;

  if (log == NULL) {
    return false;
  }

  if (!hubos_audit_log_reserve(log, log->count + 1)) {
    return false;
  }

  event.sequence = log->next_sequence++;
  event.type = type;
  event.subject_id = subject_id;
  event.resource_id = resource_id;
  event.related_id = related_id;
  event.value = value;
  log->items[log->count++] = event;
  return true;
}

const hubos_audit_event_t *hubos_audit_log_get(const hubos_audit_log_t *log, size_t index) {
  if (log == NULL || index >= log->count) {
    return NULL;
  }

  return &log->items[index];
}

size_t hubos_audit_log_count(const hubos_audit_log_t *log) {
  return log != NULL ? log->count : 0;
}

void hubos_audit_log_clear(hubos_audit_log_t *log) {
  if (log == NULL) {
    return;
  }

  log->count = 0;
}
