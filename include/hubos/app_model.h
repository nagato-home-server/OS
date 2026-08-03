#ifndef HUBOS_APP_MODEL_H
#define HUBOS_APP_MODEL_H

#include "hubos/model.h"

typedef enum {
  HUBOS_APP_SMALL = 0,
  HUBOS_APP_SERVICE,
  HUBOS_APP_CONTAINER,
  HUBOS_APP_VM,
  HUBOS_APP_LARGE,
} hubos_app_kind_t;

typedef struct {
  size_t memory;
  size_t hugepages;
  size_t cpu_cores;
  size_t scheduling_budget;
  size_t numa_node;
  size_t io_sessions;
  size_t device_sessions;
} hubos_resource_envelope_t;

typedef struct {
  hubos_id_t id;
  hubos_app_kind_t kind;
  hubos_resource_envelope_t envelope;
} hubos_app_t;

static inline void hubos_resource_envelope_init(hubos_resource_envelope_t *envelope,
                                                size_t memory,
                                                size_t hugepages,
                                                size_t cpu_cores,
                                                size_t scheduling_budget,
                                                size_t numa_node,
                                                size_t io_sessions,
                                                size_t device_sessions) {
  if (envelope == NULL) {
    return;
  }

  envelope->memory = memory;
  envelope->hugepages = hugepages;
  envelope->cpu_cores = cpu_cores;
  envelope->scheduling_budget = scheduling_budget;
  envelope->numa_node = numa_node;
  envelope->io_sessions = io_sessions;
  envelope->device_sessions = device_sessions;
}

static inline void hubos_app_init(hubos_app_t *app,
                                  hubos_id_t id,
                                  hubos_app_kind_t kind,
                                  hubos_resource_envelope_t envelope) {
  if (app == NULL) {
    return;
  }

  app->id = id;
  app->kind = kind;
  app->envelope = envelope;
}

#endif
