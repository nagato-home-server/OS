#ifndef HUBOS_MICROKIT_GENERATED_H
#define HUBOS_MICROKIT_GENERATED_H

#include <microkit.h>

#include "hubos/microkit_boot.h"
#include "hubos/microkit_runtime.h"

typedef struct {
  hubos_microkit_runtime_t runtime;
} hubos_microkit_generated_t;

typedef enum {
  HUBOS_MICROKIT_ENTRYPOINT_BOOTSTRAP = 0,
  HUBOS_MICROKIT_ENTRYPOINT_ROOT_TASK,
  HUBOS_MICROKIT_ENTRYPOINT_SERVICE,
  HUBOS_MICROKIT_ENTRYPOINT_NOTIFICATION,
} hubos_microkit_entrypoint_kind_t;

void hubos_microkit_generated_init(hubos_system_t *system);
void hubos_microkit_generated_reset(void);

bool hubos_microkit_generated_bootstrap(void);
bool hubos_microkit_generated_bootstrap_entrypoint(void);
bool hubos_microkit_generated_is_bootstrapped(void);
bool hubos_microkit_generated_validate(void);

const hubos_microkit_generated_t *hubos_microkit_generated_state(void);
const hubos_microkit_boot_manifest_t *hubos_microkit_generated_manifest(void);
const hubos_microkit_boot_component_t *hubos_microkit_generated_component(unsigned badge);
const hubos_microkit_endpoint_binding_t *hubos_microkit_generated_binding(unsigned badge);
size_t hubos_microkit_generated_publishable_endpoint_count(void);

bool hubos_microkit_generated_dispatch_root_task(
  unsigned badge,
  const hubos_root_task_request_t *request,
  hubos_root_task_response_t *response);

bool hubos_microkit_generated_dispatch_root_task_entrypoint(
  unsigned badge,
  const hubos_root_task_request_t *request,
  hubos_root_task_response_t *response);

bool hubos_microkit_generated_dispatch_service(unsigned badge,
                                               const hubos_microkit_ipc_request_t *request,
                                               hubos_microkit_ipc_response_t *response);

bool hubos_microkit_generated_dispatch_protected(unsigned badge,
                                                 microkit_msginfo msginfo,
                                                 microkit_msginfo *reply_msginfo);

bool hubos_microkit_generated_dispatch_notification(unsigned badge);

bool hubos_microkit_generated_dispatch_notification_entrypoint(unsigned badge);

bool hubos_microkit_generated_dispatch_service_entrypoint(
  unsigned badge,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response);

bool hubos_microkit_generated_dispatch_entrypoint(
  unsigned badge,
  hubos_microkit_entrypoint_kind_t kind,
  const void *request,
  void *response);

void microkit_main(void);

#endif
