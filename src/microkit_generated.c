#include "hubos/microkit_generated.h"
#include "hubos/microkit_kernel_glue.h"

static hubos_microkit_generated_t hubos_microkit_generated_singleton;

void hubos_microkit_generated_init(hubos_system_t *system) {
  hubos_microkit_runtime_init(&hubos_microkit_generated_singleton.runtime, system);
}

void hubos_microkit_generated_reset(void) {
  hubos_microkit_runtime_destroy(&hubos_microkit_generated_singleton.runtime);
}

bool hubos_microkit_generated_bootstrap(void) {
  return hubos_microkit_kernel_bootstrap(&hubos_microkit_generated_singleton.runtime);
}

bool hubos_microkit_generated_bootstrap_entrypoint(void) {
  return hubos_microkit_kernel_dispatch_entrypoint(&hubos_microkit_generated_singleton.runtime,
                                                   0,
                                                   HUBOS_MICROKIT_ENTRYPOINT_BOOTSTRAP,
                                                   NULL,
                                                   NULL);
}

bool hubos_microkit_generated_is_bootstrapped(void) {
  return hubos_microkit_runtime_is_bootstrapped(&hubos_microkit_generated_singleton.runtime);
}

bool hubos_microkit_generated_validate(void) {
  return hubos_microkit_runtime_validate(&hubos_microkit_generated_singleton.runtime);
}

const hubos_microkit_generated_t *hubos_microkit_generated_state(void) {
  return &hubos_microkit_generated_singleton;
}

const hubos_microkit_boot_manifest_t *hubos_microkit_generated_manifest(void) {
  if (hubos_microkit_generated_singleton.runtime.system == NULL) {
    return NULL;
  }

  return hubos_system_microkit_boot_manifest(hubos_microkit_generated_singleton.runtime.system);
}

const hubos_microkit_boot_component_t *hubos_microkit_generated_component(unsigned badge) {
  const hubos_microkit_boot_manifest_t *manifest = hubos_microkit_generated_manifest();

  if (manifest == NULL) {
    return NULL;
  }

  return hubos_microkit_boot_manifest_get_by_badge(manifest, badge);
}

const hubos_microkit_endpoint_binding_t *hubos_microkit_generated_binding(unsigned badge) {
  return hubos_microkit_runtime_binding(&hubos_microkit_generated_singleton.runtime, badge);
}

size_t hubos_microkit_generated_publishable_endpoint_count(void) {
  const hubos_microkit_boot_manifest_t *manifest = hubos_microkit_generated_manifest();

  if (manifest == NULL) {
    return 0;
  }

  return hubos_microkit_boot_manifest_publishable_endpoint_count(manifest);
}

bool hubos_microkit_generated_dispatch_root_task(
  unsigned badge,
  const hubos_root_task_request_t *request,
  hubos_root_task_response_t *response) {
  return hubos_microkit_kernel_dispatch_root_task(&hubos_microkit_generated_singleton.runtime,
                                                  badge,
                                                  request,
                                                  response);
}

bool hubos_microkit_generated_dispatch_root_task_entrypoint(
  unsigned badge,
  const hubos_root_task_request_t *request,
  hubos_root_task_response_t *response) {
  return hubos_microkit_kernel_dispatch_entrypoint(&hubos_microkit_generated_singleton.runtime,
                                                   badge,
                                                   HUBOS_MICROKIT_ENTRYPOINT_ROOT_TASK,
                                                   request,
                                                   response);
}

bool hubos_microkit_generated_dispatch_service(unsigned badge,
                                               const hubos_microkit_ipc_request_t *request,
                                               hubos_microkit_ipc_response_t *response) {
  return hubos_microkit_kernel_dispatch_service(&hubos_microkit_generated_singleton.runtime,
                                                badge,
                                                request,
                                                response);
}

bool hubos_microkit_generated_dispatch_protected(unsigned badge,
                                                 microkit_msginfo msginfo,
                                                 microkit_msginfo *reply_msginfo) {
  bool ok;

  if (reply_msginfo == NULL) {
    return false;
  }

  *reply_msginfo = microkit_msginfo_new(0, 0);

  ok = hubos_microkit_kernel_dispatch_protected(&hubos_microkit_generated_singleton.runtime,
                                                badge,
                                                msginfo,
                                                reply_msginfo);
  if (!ok) {
    *reply_msginfo = microkit_msginfo_new(0, 0);
  }

  return ok;
}

bool hubos_microkit_generated_dispatch_notification(unsigned badge) {
  return hubos_microkit_kernel_dispatch_notification(&hubos_microkit_generated_singleton.runtime,
                                                     badge);
}

bool hubos_microkit_generated_dispatch_notification_entrypoint(unsigned badge) {
  return hubos_microkit_kernel_dispatch_entrypoint(&hubos_microkit_generated_singleton.runtime,
                                                   badge,
                                                   HUBOS_MICROKIT_ENTRYPOINT_NOTIFICATION,
                                                   NULL,
                                                   NULL);
}

bool hubos_microkit_generated_dispatch_service_entrypoint(
  unsigned badge,
  const hubos_microkit_ipc_request_t *request,
  hubos_microkit_ipc_response_t *response) {
  return hubos_microkit_kernel_dispatch_entrypoint(&hubos_microkit_generated_singleton.runtime,
                                                   badge,
                                                   HUBOS_MICROKIT_ENTRYPOINT_SERVICE,
                                                   request,
                                                   response);
}

bool hubos_microkit_generated_dispatch_entrypoint(
  unsigned badge,
  hubos_microkit_entrypoint_kind_t kind,
  const void *request,
  void *response) {
  return hubos_microkit_kernel_dispatch_entrypoint(&hubos_microkit_generated_singleton.runtime,
                                                   badge,
                                                   kind,
                                                   request,
                                                   response);
}

void microkit_main(void) {
  if (!hubos_microkit_generated_validate()) {
    return;
  }

  (void)hubos_microkit_generated_bootstrap_entrypoint();
}
