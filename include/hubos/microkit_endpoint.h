#ifndef HUBOS_MICROKIT_ENDPOINT_H
#define HUBOS_MICROKIT_ENDPOINT_H

#include "hubos/microkit_ipc.h"
#include "hubos/root_task.h"

bool hubos_microkit_endpoint_dispatch(const hubos_microkit_endpoint_binding_t *binding,
                                     hubos_system_t *system,
                                     const hubos_microkit_ipc_request_t *request,
                                     hubos_microkit_ipc_response_t *response);

bool hubos_microkit_endpoint_dispatch_by_badge(const hubos_microkit_ipc_layout_t *layout,
                                                hubos_system_t *system,
                                                unsigned badge,
                                                const hubos_microkit_ipc_request_t *request,
                                                hubos_microkit_ipc_response_t *response);

bool hubos_microkit_root_task_dispatch(const hubos_microkit_endpoint_binding_t *binding,
                                       hubos_root_task_t *root_task,
                                       const hubos_root_task_request_t *request,
                                       hubos_root_task_response_t *response);

bool hubos_microkit_root_task_dispatch_by_badge(const hubos_microkit_ipc_layout_t *layout,
                                                hubos_root_task_t *root_task,
                                                unsigned badge,
                                                const hubos_root_task_request_t *request,
                                                hubos_root_task_response_t *response);

#endif
