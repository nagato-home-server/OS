#include "hubos/microkit_kernel_glue.h"
#include "hubos/microkit_transport.h"

bool hubos_microkit_kernel_bootstrap(hubos_microkit_runtime_t *runtime) {
  return hubos_microkit_runtime_bootstrap(runtime);
}

bool hubos_microkit_kernel_dispatch_root_task(hubos_microkit_runtime_t *runtime,
                                              unsigned badge,
                                              const hubos_root_task_request_t *request,
                                              hubos_root_task_response_t *response) {
  return hubos_microkit_runtime_dispatch_root_task(runtime, badge, request, response);
}

bool hubos_microkit_kernel_dispatch_service(hubos_microkit_runtime_t *runtime,
                                            unsigned badge,
                                            const hubos_microkit_ipc_request_t *request,
                                            hubos_microkit_ipc_response_t *response) {
  return hubos_microkit_runtime_dispatch_service(runtime, badge, request, response);
}

bool hubos_microkit_kernel_dispatch_protected(hubos_microkit_runtime_t *runtime,
                                              unsigned badge,
                                              microkit_msginfo msginfo,
                                              microkit_msginfo *reply_msginfo) {
  const hubos_microkit_endpoint_binding_t *binding = NULL;
  hubos_microkit_transport_frame_t request_frame;
  hubos_microkit_transport_frame_t response_frame;
  hubos_microkit_ipc_request_t request;
  hubos_microkit_ipc_response_t response;

  if (reply_msginfo == NULL) {
    return false;
  }

  *reply_msginfo = microkit_msginfo_new(0, 0);

  if (runtime == NULL || runtime->system == NULL) {
    return false;
  }

  if (microkit_msginfo_get_label(msginfo) != HUBOS_MICROKIT_TRANSPORT_LABEL) {
    return false;
  }

  binding = hubos_microkit_runtime_binding(runtime, badge);
  if (binding == NULL || !binding->exposed) {
    return false;
  }

  hubos_microkit_transport_frame_init(&request_frame);
  if (!hubos_microkit_transport_frame_from_msginfo(&request_frame, msginfo)) {
    return false;
  }

  if (!hubos_microkit_transport_request_decode(&request_frame, &request)) {
    return false;
  }

  if (binding->component != request.service) {
    return false;
  }

  if (!hubos_microkit_runtime_dispatch_service(runtime, badge, &request, &response)) {
    return false;
  }

  hubos_microkit_transport_frame_init(&response_frame);
  if (!hubos_microkit_transport_response_encode(&response, &response_frame)) {
    return false;
  }

  hubos_microkit_transport_frame_to_mrs(&response_frame);
  *reply_msginfo =
    hubos_microkit_transport_frame_to_msginfo(&response_frame,
                                              HUBOS_MICROKIT_TRANSPORT_LABEL);
  return true;
}

bool hubos_microkit_kernel_dispatch_notification(hubos_microkit_runtime_t *runtime,
                                                 unsigned badge) {
  return hubos_microkit_runtime_dispatch_notification(runtime, badge);
}

bool hubos_microkit_kernel_dispatch_entrypoint(hubos_microkit_runtime_t *runtime,
                                               unsigned badge,
                                               hubos_microkit_entrypoint_kind_t kind,
                                               const void *request,
                                               void *response) {
  if (runtime == NULL) {
    return false;
  }

  switch (kind) {
  case HUBOS_MICROKIT_ENTRYPOINT_BOOTSTRAP:
    return hubos_microkit_kernel_bootstrap(runtime);
  case HUBOS_MICROKIT_ENTRYPOINT_ROOT_TASK:
    return hubos_microkit_kernel_dispatch_root_task(runtime,
                                                    badge,
                                                    (const hubos_root_task_request_t *)request,
                                                    (hubos_root_task_response_t *)response);
  case HUBOS_MICROKIT_ENTRYPOINT_SERVICE:
    return hubos_microkit_kernel_dispatch_service(runtime,
                                                  badge,
                                                  (const hubos_microkit_ipc_request_t *)request,
                                                  (hubos_microkit_ipc_response_t *)response);
  case HUBOS_MICROKIT_ENTRYPOINT_NOTIFICATION:
    return hubos_microkit_kernel_dispatch_notification(runtime, badge);
  }

  return false;
}
