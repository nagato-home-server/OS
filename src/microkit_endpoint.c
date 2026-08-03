#include "hubos/microkit_endpoint.h"

static bool hubos_microkit_endpoint_binding_matches(
  const hubos_microkit_endpoint_binding_t *binding,
  hubos_microkit_component_kind_t component) {
  return binding != NULL && binding->component == component;
}

bool hubos_microkit_endpoint_dispatch(const hubos_microkit_endpoint_binding_t *binding,
                                     hubos_system_t *system,
                                     const hubos_microkit_ipc_request_t *request,
                                     hubos_microkit_ipc_response_t *response) {
  if (binding == NULL || system == NULL || request == NULL || response == NULL || !binding->exposed) {
    return false;
  }

  if (!hubos_microkit_endpoint_binding_matches(binding, request->service)) {
    return false;
  }

  return hubos_system_microkit_ipc_dispatch(system, request, response);
}

bool hubos_microkit_endpoint_dispatch_by_badge(const hubos_microkit_ipc_layout_t *layout,
                                                hubos_system_t *system,
                                                unsigned badge,
                                                const hubos_microkit_ipc_request_t *request,
                                                hubos_microkit_ipc_response_t *response) {
  const hubos_microkit_endpoint_binding_t *binding = NULL;

  if (layout == NULL) {
    return false;
  }

  binding = hubos_microkit_ipc_layout_get_by_badge(layout, badge);
  return hubos_microkit_endpoint_dispatch(binding, system, request, response);
}

bool hubos_microkit_root_task_dispatch(const hubos_microkit_endpoint_binding_t *binding,
                                       hubos_root_task_t *root_task,
                                       const hubos_root_task_request_t *request,
                                       hubos_root_task_response_t *response) {
  if (binding == NULL || root_task == NULL || request == NULL || response == NULL || !binding->exposed) {
    return false;
  }

  if (!hubos_microkit_endpoint_binding_matches(binding, HUBOS_MICROKIT_COMPONENT_ROOT_TASK)) {
    return false;
  }

  return hubos_root_task_dispatch(root_task, request, response);
}

bool hubos_microkit_root_task_dispatch_by_badge(const hubos_microkit_ipc_layout_t *layout,
                                                hubos_root_task_t *root_task,
                                                unsigned badge,
                                                const hubos_root_task_request_t *request,
                                                hubos_root_task_response_t *response) {
  const hubos_microkit_endpoint_binding_t *binding = NULL;

  if (layout == NULL) {
    return false;
  }

  binding = hubos_microkit_ipc_layout_get_by_badge(layout, badge);
  return hubos_microkit_root_task_dispatch(binding, root_task, request, response);
}
