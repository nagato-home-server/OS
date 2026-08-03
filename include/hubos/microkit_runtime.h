#ifndef HUBOS_MICROKIT_RUNTIME_H
#define HUBOS_MICROKIT_RUNTIME_H

#include "hubos/microkit_boot.h"
#include "hubos/microkit_endpoint.h"

typedef struct {
  hubos_system_t *system;
  hubos_root_task_t root_task;
  bool bootstrapped;
} hubos_microkit_runtime_t;

void hubos_microkit_runtime_init(hubos_microkit_runtime_t *runtime, hubos_system_t *system);
void hubos_microkit_runtime_destroy(hubos_microkit_runtime_t *runtime);

const hubos_microkit_ipc_layout_t *hubos_microkit_runtime_layout(
  const hubos_microkit_runtime_t *runtime);
const hubos_microkit_endpoint_binding_t *hubos_microkit_runtime_binding(
  const hubos_microkit_runtime_t *runtime,
  unsigned badge);
bool hubos_microkit_runtime_validate(const hubos_microkit_runtime_t *runtime);
bool hubos_microkit_runtime_bootstrap(hubos_microkit_runtime_t *runtime);
bool hubos_microkit_runtime_is_bootstrapped(const hubos_microkit_runtime_t *runtime);

bool hubos_microkit_runtime_dispatch_root_task(hubos_microkit_runtime_t *runtime,
                                               unsigned badge,
                                               const hubos_root_task_request_t *request,
                                               hubos_root_task_response_t *response);
bool hubos_microkit_runtime_dispatch_service(hubos_microkit_runtime_t *runtime,
                                            unsigned badge,
                                            const hubos_microkit_ipc_request_t *request,
                                            hubos_microkit_ipc_response_t *response);
bool hubos_microkit_runtime_dispatch_notification(hubos_microkit_runtime_t *runtime,
                                                  unsigned badge);

#endif
