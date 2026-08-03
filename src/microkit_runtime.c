#include "hubos/microkit_runtime.h"

void hubos_microkit_runtime_init(hubos_microkit_runtime_t *runtime, hubos_system_t *system) {
  if (runtime == NULL) {
    return;
  }

  runtime->system = system;
  runtime->bootstrapped = false;
  hubos_root_task_init(&runtime->root_task, system);
}

void hubos_microkit_runtime_destroy(hubos_microkit_runtime_t *runtime) {
  if (runtime == NULL) {
    return;
  }

  runtime->system = NULL;
  runtime->bootstrapped = false;
  hubos_root_task_init(&runtime->root_task, NULL);
}

const hubos_microkit_ipc_layout_t *hubos_microkit_runtime_layout(
  const hubos_microkit_runtime_t *runtime) {
  if (runtime == NULL || runtime->system == NULL) {
    return NULL;
  }

  return hubos_system_microkit_ipc_layout(runtime->system);
}

const hubos_microkit_endpoint_binding_t *hubos_microkit_runtime_binding(
  const hubos_microkit_runtime_t *runtime,
  unsigned badge) {
  const hubos_microkit_ipc_layout_t *layout = hubos_microkit_runtime_layout(runtime);

  if (layout == NULL) {
    return NULL;
  }

  return hubos_microkit_ipc_layout_get_by_badge(layout, badge);
}

bool hubos_microkit_runtime_validate(const hubos_microkit_runtime_t *runtime) {
  if (runtime == NULL || runtime->system == NULL) {
    return false;
  }

  return hubos_system_microkit_graph_validate(runtime->system) &&
         hubos_system_microkit_boot_manifest_validate(runtime->system) &&
         hubos_system_microkit_ipc_layout_validate(runtime->system) &&
         hubos_system_boot_capabilities_validate(runtime->system);
}

bool hubos_microkit_runtime_bootstrap(hubos_microkit_runtime_t *runtime) {
  if (runtime == NULL || runtime->system == NULL || runtime->bootstrapped) {
    return false;
  }

  if (!hubos_microkit_runtime_validate(runtime)) {
    return false;
  }

  if (!hubos_root_task_bootstrap(&runtime->root_task)) {
    return false;
  }

  runtime->bootstrapped = true;
  return true;
}

bool hubos_microkit_runtime_is_bootstrapped(const hubos_microkit_runtime_t *runtime) {
  return runtime != NULL && runtime->bootstrapped && hubos_root_task_is_dormant(&runtime->root_task);
}

bool hubos_microkit_runtime_dispatch_root_task(hubos_microkit_runtime_t *runtime,
                                               unsigned badge,
                                               const hubos_root_task_request_t *request,
                                               hubos_root_task_response_t *response) {
  if (runtime == NULL || runtime->system == NULL || request == NULL || response == NULL) {
    return false;
  }

  return hubos_microkit_root_task_dispatch_by_badge(hubos_microkit_runtime_layout(runtime),
                                                   &runtime->root_task,
                                                   badge,
                                                   request,
                                                   response);
}

bool hubos_microkit_runtime_dispatch_service(hubos_microkit_runtime_t *runtime,
                                            unsigned badge,
                                            const hubos_microkit_ipc_request_t *request,
                                            hubos_microkit_ipc_response_t *response) {
  if (runtime == NULL || runtime->system == NULL || request == NULL || response == NULL) {
    return false;
  }

  if (!hubos_microkit_runtime_is_bootstrapped(runtime)) {
    return false;
  }

  return hubos_microkit_endpoint_dispatch_by_badge(hubos_microkit_runtime_layout(runtime),
                                                   runtime->system,
                                                   badge,
                                                   request,
                                                   response);
}

bool hubos_microkit_runtime_dispatch_notification(hubos_microkit_runtime_t *runtime,
                                                  unsigned badge) {
  const hubos_microkit_boot_manifest_t *manifest = NULL;
  const hubos_microkit_boot_component_t *component = NULL;

  if (runtime == NULL || runtime->system == NULL) {
    return false;
  }

  if (!hubos_microkit_runtime_is_bootstrapped(runtime)) {
    return false;
  }

  manifest = hubos_system_microkit_boot_manifest(runtime->system);
  if (manifest == NULL) {
    return false;
  }

  component = hubos_microkit_boot_manifest_get_by_badge(manifest, badge);
  if (component == NULL || !component->notification_published) {
    return false;
  }

  return true;
}
