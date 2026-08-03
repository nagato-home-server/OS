#ifndef HUBOS_MICROKIT_KERNEL_GLUE_H
#define HUBOS_MICROKIT_KERNEL_GLUE_H

#include <microkit.h>

#include "hubos/microkit_generated.h"

bool hubos_microkit_kernel_bootstrap(hubos_microkit_runtime_t *runtime);
bool hubos_microkit_kernel_dispatch_root_task(hubos_microkit_runtime_t *runtime,
                                              unsigned badge,
                                              const hubos_root_task_request_t *request,
                                              hubos_root_task_response_t *response);
bool hubos_microkit_kernel_dispatch_service(hubos_microkit_runtime_t *runtime,
                                            unsigned badge,
                                            const hubos_microkit_ipc_request_t *request,
                                            hubos_microkit_ipc_response_t *response);
bool hubos_microkit_kernel_dispatch_protected(hubos_microkit_runtime_t *runtime,
                                              unsigned badge,
                                              microkit_msginfo msginfo,
                                              microkit_msginfo *reply_msginfo);
bool hubos_microkit_kernel_dispatch_notification(hubos_microkit_runtime_t *runtime,
                                                 unsigned badge);
bool hubos_microkit_kernel_dispatch_entrypoint(hubos_microkit_runtime_t *runtime,
                                               unsigned badge,
                                               hubos_microkit_entrypoint_kind_t kind,
                                               const void *request,
                                               void *response);

#endif
