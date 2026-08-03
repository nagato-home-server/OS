#ifndef HUBOS_ROOT_TASK_H
#define HUBOS_ROOT_TASK_H

#include "hubos/ipc.h"
#include "hubos/system.h"

typedef struct {
  hubos_system_t *system;
  bool bootstrapped;
  bool dormant;
  hubos_id_t hub_resource_id;
  hubos_id_t resource_registry_resource_id;
  hubos_id_t root_session_id;
  hubos_id_t root_capability_id;
  hubos_id_t driver_registry_resource_id;
  hubos_id_t bootstrap_driver_id;
} hubos_root_task_t;

void hubos_root_task_init(hubos_root_task_t *root_task, hubos_system_t *system);
bool hubos_root_task_bootstrap(hubos_root_task_t *root_task);
bool hubos_root_task_is_dormant(const hubos_root_task_t *root_task);

bool hubos_root_task_dispatch(hubos_root_task_t *root_task,
                              const hubos_root_task_request_t *request,
                              hubos_root_task_response_t *response);

#endif
